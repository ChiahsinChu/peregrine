#ifndef GRAPH_HH
#define GRAPH_HH

#include <execution>
#include <map>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <set>
#include <iterator>
#include <queue>
#include <cmath>
#include "PO.hh"
#include "utils.hh"
#include "./bliss-0.73/graph.hh"


namespace Peregrine
{

  // forward declaration to allow friendship with SmallGraph
  struct PatternGenerator;
  class DataGraph;
  
  namespace Graph
  {
    enum Labelling
    {
      UNLABELLED,
      LABELLED,
      PARTIALLY_LABELLED,
      DISCOVER_LABELS
    };
  } // namespace Graph

  class SmallGraph
  {
    private:
      friend class AnalyzedPattern;
      friend struct ::Peregrine::PatternGenerator;
      friend class ::Peregrine::DataGraph;
      std::unordered_map<uint32_t, std::vector<uint32_t>> true_adj_list;
      std::unordered_map<uint32_t, std::vector<uint32_t>> anti_adj_list;
      std::vector<uint32_t> labels;
      Graph::Labelling labelling = Graph::UNLABELLED;
    public:
      friend std::ostream &operator <<(std::ostream &os, const SmallGraph &p )
      {
        os << p.to_string();
        return os;
      }

      uint32_t num_vertices() const
      {
        return true_adj_list.size();
      }

      uint32_t num_anti_vertices() const
      {
        uint32_t c = 0;
        for (const auto &[u, _] : anti_adj_list)
        {
          if (!true_adj_list.contains(u)) c += 1;
        }
        return c;
      }

      const std::vector<uint32_t> &get_neighbours(uint32_t v) const
      {
        return true_adj_list.at(v);
      }

      const std::vector<uint32_t> &get_anti_neighbours(uint32_t v) const
      {
        return anti_adj_list.at(v);
      }

      uint32_t num_true_edges() const
      {
        uint32_t count = 0;
        for (const auto &[u, nbrs] : true_adj_list)
        {
          count += nbrs.size();
        }

        return count / 2;
      }

      uint32_t num_anti_edges() const
      {
        uint32_t count = 0;
        for (const auto &[u, nbrs] : anti_adj_list)
        {
          count += nbrs.size();
        }

        return count / 2;
      }

      std::vector<uint32_t> v_list() const
      {
        std::vector<uint32_t> vs;
        for (const auto &[v, _] : true_adj_list) vs.push_back(v);

        std::sort(std::execution::unseq, vs.begin(), vs.end());
        return vs;
      }

      const std::vector<uint32_t> &get_labels() const
      {
        return labels;
      }

      uint32_t label(uint32_t qv) const
      {
        return labels[qv-1];
      }

      void set_labelling(Graph::Labelling l)
      {
        labelling = l;
      }

      Graph::Labelling get_labelling() const
      {
        return labelling;
      }

      /**
       * Only works for undirected patterns with only true edges/vertices
       */
      uint64_t bliss_hash() const {
        bliss::Graph bliss_qg;
        if (labelling == Graph::LABELLED || labelling == Graph::PARTIALLY_LABELLED)
        {
          for (size_t i = 0; i < num_vertices(); i++)
          {
              bliss_qg.add_vertex(labels[i]);
          }
        }
        else
        {
          for (size_t i = 0; i < num_vertices(); i++)
          {
              bliss_qg.add_vertex();
          }
        }

        for (const auto &[u, nbrs] : true_adj_list)
        {
          for (uint32_t v : nbrs)
          {
            if (u < v)
            {
              bliss_qg.add_edge(u - 1, v - 1);
            }
          }
        }

        bliss::Stats s;
        auto *new_graph = bliss_qg.permute(bliss_qg.canonical_form(s, NULL, NULL));

        return new_graph->get_hash();
      }

      std::string to_string(const std::vector<uint32_t> &given_labels) const
      {
        if (labelling == Graph::LABELLED || labelling == Graph::PARTIALLY_LABELLED)
        {
          assert(given_labels.size() >= num_vertices() + num_anti_vertices());
          std::string res("");
          for (const auto &[u, nbrs] : true_adj_list)
          {
            auto l1 = given_labels[u-1] == static_cast<uint32_t>(-1)
              ? "*" : std::to_string(given_labels[u-1]);
            for (uint32_t v : nbrs)
            {
              if (u > v) continue;
              auto l2 = given_labels[v-1] == static_cast<uint32_t>(-1)
                ? "*" : std::to_string(given_labels[v-1]);

              res += "[";
              res += std::to_string(u) + "," + l1;
              res += "-";
              res += std::to_string(v) + "," + l2;
              res += "]";
            }
          }

          for (const auto &[u, nbrs] : anti_adj_list)
          {
            auto l1 = given_labels[u-1] == static_cast<uint32_t>(-1)
              ? "*" : std::to_string(given_labels[u-1]);
            for (uint32_t v : nbrs)
            {
              if (u > v) continue;
              auto l2 = given_labels[v-1] == static_cast<uint32_t>(-1)
                ? "*" : std::to_string(given_labels[v-1]);

              res += "(";
              res += std::to_string(u) + "," + l1;
              res += "~";
              res += std::to_string(v) + "," + l2;
              res += ")";
            }
          }
          return res;
        } else {
          return to_string();
        }
      }

      std::string to_string() const {
        if (labelling == Graph::LABELLED || labelling == Graph::PARTIALLY_LABELLED)
        {
          return to_string(labels);
        } else {
          std::string res("");
          for (const auto &[u, nbrs] : true_adj_list)
          {
            for (uint32_t v : nbrs)
            {
              if (u > v) continue;
              res += "[";
              res += std::to_string(u);
              res += "-";
              res += std::to_string(v);
              res += "]";
            }
          }

          for (const auto &[u, nbrs] : anti_adj_list)
          {
            for (uint32_t v : nbrs)
            {
              if (u > v) continue;
              res += "(";
              res += std::to_string(u);
              res += "~";
              res += std::to_string(v);
              res += ")";
            }
          }
          return res;
        }
      }

      SmallGraph() {}

      SmallGraph(const SmallGraph &other, const std::vector<uint32_t> &labels)
        : true_adj_list(other.true_adj_list),
          anti_adj_list(other.anti_adj_list),
          labels(labels),
          labelling(determine_labelling())
        {
        }

      Graph::Labelling determine_labelling() const
      {
        if (labels.size() == num_vertices() + num_anti_vertices())
        {
          Graph::Labelling l = Graph::LABELLED;

          uint32_t v = 1;
          for (uint32_t l : labels)
          {
            if (l == static_cast<uint32_t>(-1) && !is_anti_vertex(v))
            {
              l = Graph::PARTIALLY_LABELLED;
            }
            v += 1;
          }

          return l;
        }
        else
        {
          return Graph::UNLABELLED;
        }
      }

      SmallGraph(std::vector<std::pair<uint32_t,uint32_t>> iedge_list)
        : labelling(Graph::UNLABELLED)
      {
          for (const auto &edge : iedge_list) {

              true_adj_list[edge.first].push_back(edge.second);
              true_adj_list[edge.second].push_back(edge.first);
          }
          labels.resize(num_vertices());

          // make sure anti_adj_list.at() doesn't fail
          for (uint32_t v = 1; v <= num_vertices(); ++v) anti_adj_list[v];
      }

      /**
       * Note: no anti-edges are passed!
       */
      SmallGraph(const std::unordered_map<uint32_t, std::vector<uint32_t>> &adj,
          const std::vector<uint32_t> &labels = {0})
        : true_adj_list(adj),
          labels(labels)
      {
        if (labels.size() != adj.size())
        {
          labelling = Graph::UNLABELLED;
        }
        else if (utils::search(labels, static_cast<uint32_t>(-1)))
        {
          labelling = Graph::PARTIALLY_LABELLED;
        }
        else
        {
          labelling = Graph::LABELLED;
        }

        // make sure anti_adj_list.at() doesn't fail
        for (uint32_t v = 1; v <= num_vertices(); ++v) anti_adj_list[v];
      }

      SmallGraph(std::string inputfile)
      {
          std::ifstream query_graph(inputfile.c_str());
          std::string line;
          while (std::getline(query_graph, line))
          {
              std::istringstream iss(line);
              std::vector<uint32_t> vs(std::istream_iterator<uint32_t>{iss}, std::istream_iterator<uint32_t>());

              uint32_t a, b;
              if (vs.size() == 2)
              {
                a = vs[0]; b = vs[1];
                true_adj_list[a].push_back(b);
                true_adj_list[b].push_back(a);
              } else if (vs.size() == 3) { // anti-edge
                a = vs[0]; b = vs[1];
                anti_adj_list[a].push_back(b);
                anti_adj_list[b].push_back(a);
              } else if (vs.size() == 4) { // edge with labelled vertices
                labelling = Graph::LABELLED;
                uint32_t alabel, blabel;
                a = vs[0]; b = vs[2];
                alabel = vs[1]; blabel = vs[3];
                true_adj_list[a].push_back(b);
                true_adj_list[b].push_back(a);
                labels.resize(std::max({(uint32_t)labels.size(), a, b}));
                labels[a-1] = alabel;
                labels[b-1] = blabel;
              } else if (vs.size() == 5) { // anti-edge with labelled vertices
                labelling = Graph::LABELLED;
                uint32_t alabel, blabel;
                a = vs[0]; b = vs[2];
                alabel = vs[1]; blabel = vs[3];
                anti_adj_list[a].push_back(b);
                anti_adj_list[b].push_back(a);
                labels.resize(std::max({(uint32_t)labels.size(), a, b}));
                labels[a-1] = alabel;
                labels[b-1] = blabel;
              }
          }

          if (num_vertices() == 0) {
            throw std::invalid_argument("Found 0 vertices in file '" + inputfile + "'");
          }

          // check if labelled or partially labelled
          if (utils::search(labels, static_cast<uint32_t>(-1)))
          {
            labelling = Graph::PARTIALLY_LABELLED;
          }

          // make sure anti_adj_list.at() doesn't fail
          for (uint32_t v = 1; v <= num_vertices(); ++v) anti_adj_list[v];
      }

      /**
       * unchecked! may result in duplicate/invalid edges
       */
      SmallGraph &add_anti_edge(uint32_t u, uint32_t v)
      {
        anti_adj_list[u].push_back(v);
        anti_adj_list[v].push_back(u);

        if (labelling == Graph::PARTIALLY_LABELLED || labelling == Graph::LABELLED)
        {
          // may have added a anti-vertex: in which case we need to give it a label
          if (v > num_vertices())
          {
            labels.push_back(static_cast<uint32_t>(-1));
          }
        }

        return *this;
      }

      /**
       * unchecked! may result in duplicate/invalid edges
       */
      SmallGraph &add_edge(uint32_t u, uint32_t v)
      {
        true_adj_list[u].push_back(v);
        true_adj_list[v].push_back(u);

        if (labelling == Graph::PARTIALLY_LABELLED || labelling == Graph::LABELLED)
        {
          // may have added a anti-vertex: in which case we need to give it a label
          if (v > num_vertices())
          {
            labels.push_back(static_cast<uint32_t>(-3)); // just some random label
          }
        }

        return *this;
      }

      SmallGraph &set_label(uint32_t u, uint32_t l)
      {
        if (labelling == Graph::UNLABELLED || labelling == Graph::DISCOVER_LABELS)
        {
          labels.resize(num_vertices() + num_anti_vertices());
        }

        labels[u-1] = l;
        labelling = l == static_cast<uint32_t>(-1) ? Graph::PARTIALLY_LABELLED : Graph::LABELLED;

        return *this;
      }

      bool is_anti_vertex(uint32_t v) const
      {
        return anti_adj_list.contains(v) && !true_adj_list.contains(v);
      }

      SmallGraph &remove_edge(uint32_t u, uint32_t v)
      {
        if (!is_anti_vertex(u) && !is_anti_vertex(v))
        {
          std::erase(true_adj_list[u], v);
          std::erase(true_adj_list[v], u);
        }
        std::erase(anti_adj_list[u], v);
        std::erase(anti_adj_list[v], u);


        return *this;
      }

      // checks labels: it is assumed that the two patterns have the same structure
      bool operator==(const SmallGraph &p) const {
        return p.labels == labels;
      }

      bool operator<(const SmallGraph &p) const {
        return p.labels < labels;
      }
  };

  class AnalyzedPattern
  {
    public:
      SmallGraph query_graph;
      std::vector<SmallGraph> vgs;

      std::vector<std::vector<std::vector<uint32_t>>> qs;
      std::vector<std::vector<uint32_t>> cond_adj;
      // important! conditions show only a subset of symmetric vertices
      std::vector<std::pair<uint32_t, uint32_t>> conditions;
      std::vector<uint32_t> aut_map;
      uint32_t nautsets = 0;

      std::vector<uint32_t> ncore_vertices;
      std::vector<uint32_t> anti_vertices;

      // vmap[i][j][k] -> ith vgs, jth vertex, kth query sequence
      std::vector<std::vector<std::vector<uint32_t>>> vmap;
      std::vector<uint32_t> global_order;
      std::vector<std::vector<uint32_t>> qo_book;
      // bounds[i] gives a list of potential bounds for ith non-core vertex
      // take the min/max of the list as necessary
      std::vector<std::vector<uint32_t>> lower_bounds;
      std::vector<std::vector<uint32_t>> upper_bounds;

      std::vector<uint32_t> sibling_groups;
      std::vector<std::vector<uint32_t>> order_groups;
      std::vector<uint32_t> candidate_idxs;

      // sets of non-core vertices with partial orderings between them, but with no orderings across sets
      std::vector<std::vector<uint32_t>> indsets;

      SmallGraph rbi_v;

      bool is_star = false;

      AnalyzedPattern() {}

      AnalyzedPattern(const SmallGraph &p)
        : query_graph(p)
      {
          check_connected();
          get_anti_vertices();
          check_anti_vertices();
          check_labels();
          conditions = get_conditions();
          build_rbi_graph();
      }

      AnalyzedPattern(std::string inputfile)
        : query_graph(inputfile)
      {
          check_connected();
          get_anti_vertices();
          check_anti_vertices();
          check_labels();
          conditions = get_conditions();
          build_rbi_graph();
      }

      Graph::Labelling labelling_type() const
      {
        return query_graph.labelling;
      }


      bool is_anti_vertex(uint32_t v) const
      {
        return query_graph.is_anti_vertex(v);
      }


      /**
       * If the pattern is edge-induced, returns whether this pattern has
       * anti-edges connected to regular vertices
       *
       * If the pattern is vertex-induced, returns false
       */
      bool has_anti_edges() const
      {
        if (anti_vertices.empty())
        {
          if (query_graph.num_anti_edges() == 0) return false;
        }
        else
        {
          uint32_t ae = 0;
          for (uint32_t av : anti_vertices)
          {
            ae += query_graph.anti_adj_list.at(av).size();
          }

          // all anti-edges come from anti-vertices
          if (ae == query_graph.num_anti_edges()) return false;
        }

        // false iff vertex-induced
        uint32_t m = query_graph.num_anti_edges() + query_graph.num_true_edges();
        uint32_t n = query_graph.num_vertices();
        return m != (n*(n-1))/2;
      }

      void check_connected() const
      {
        if (!is_connected(query_graph.v_list(), query_graph.true_adj_list))
        {
          throw std::runtime_error(query_graph.to_string() + " is not connected and cannot be matched.");
        }
      }

      void check_labels() const
      {
        if (labelling_type() == Graph::PARTIALLY_LABELLED)
        {
          uint32_t l = std::count(query_graph.labels.cbegin(),
              query_graph.labels.cend(),
              static_cast<uint32_t>(-1));

          if (l > 1)
          {
            throw std::runtime_error("Cannot match multiple unlabelled vertices in a partially labelled pattern. Try building up to this pattern from a smaller partially labelled pattern.");
          }
          else if (l < 1)
          {
            throw std::runtime_error("Partially-labelled pattern has no '*'-labelled vertex");
          }
        }
      }

      void check_anti_vertices() const
      {
        for (uint32_t av : anti_vertices)
        {
          // make sure these are actually anti-vertices
          // don't know how this could happen but can never be too careful
          if (!is_anti_vertex(av))
          {
            throw std::runtime_error(std::to_string(av) + " mistakenly classified as anti-vertex.");
          }

          // make sure anti-vertex comes after all regular vertices
          if (av <= query_graph.num_vertices())
          {
            throw std::runtime_error(std::to_string(av) + " has a smaller ID than the regular vertices");
          }

          // make sure anti-vertices aren't connected to other anti-vertices
          for (uint32_t v : query_graph.get_anti_neighbours(av))
          {
            if (is_anti_vertex(v))
            {
              throw std::runtime_error("Anti-vertex " + std::to_string(av) + " is connected to another anti-vertex " + std::to_string(v) + "; this pattern is impossible to match");
            }
          }
        }
      }

      void get_anti_vertices()
      {
        for (const auto &lst : query_graph.anti_adj_list)
        {
          uint32_t v = lst.first;
          if (is_anti_vertex(v))
          {
            anti_vertices.push_back(v);
          }
        }
      }

      bool is_clique() const
      {
        assert(query_graph.num_vertices() != 0);
        uint32_t n = query_graph.num_vertices();
        return anti_vertices.empty() && query_graph.num_true_edges() == (n*(n-1))/2;
      }

      uint32_t match_po(const std::vector<uint32_t> &v_list, const std::vector<std::pair<uint32_t, uint32_t>> &po)
      {
          uint32_t internal_po_count = 0;
          for (const auto &po_pair : po)
          {
              if (utils::search(v_list, po_pair.first) && utils::search(v_list, po_pair.second))
              {
                  internal_po_count++;
              }
          }
          return internal_po_count;
      }

      bool is_connected(const std::set<uint32_t> &vertex_set, const std::unordered_map<uint32_t, std::vector<uint32_t>> &adj_list) const
      {
          std::vector<uint32_t> vertex_cover(vertex_set.cbegin(), vertex_set.cend());
          return is_connected(vertex_cover, adj_list);
      }

      bool is_connected(const std::vector<uint32_t> &vertex_cover, const std::unordered_map<uint32_t, std::vector<uint32_t>> &adj_list) const
      {
          std::vector<std::vector<uint32_t>> graph(*max_element(vertex_cover.begin(), vertex_cover.end()) + 1);
          for (auto &v : adj_list)
          {
              graph[v.first] = v.second;
          }
          std::vector<bool> visited(graph.size() + 1, true);

          for (auto vertex_id : vertex_cover)
          {
              visited[vertex_id] = false;
          }
          std::queue<int> q;
          q.push(vertex_cover[0]);
          visited[vertex_cover[0]] = true;
          while (!q.empty())
          {
              uint32_t current_node = q.front();
              q.pop();
              for (auto vertex_id : graph[current_node])
              {
                  if (!visited[vertex_id])
                  {
                      q.push(vertex_id);
                      visited[vertex_id] = true;
                  }
              }
          }
          for (size_t i = 1; i <= graph.size(); i++)
          {
              if (!visited[i])
                  return false;
          }
          return true;
      }

      SmallGraph get_mvcv(uint32_t id, uint32_t *status)
      {
        if (is_clique() && labelling_type() == Graph::UNLABELLED)
        {
          std::vector<std::pair<uint32_t, uint32_t>> edge_list;
          for (uint32_t u = 1; u < query_graph.num_vertices(); ++u)
          {
            for (uint32_t v = u+1; v < query_graph.num_vertices(); ++v)
            {
              edge_list.emplace_back(u, v);
            }
          }

          return edge_list;
        }

        std::set<uint32_t> vertex_cover;
        // this ensures anti-vertex is never chosen so long as num_vertices()
        // counts regular vertices, since we start at num_vertices() and go down
        uint32_t vid = query_graph.num_vertices();
        while (id)
        {
            if (id % 2)
                vertex_cover.insert(vid);
            vid -= 1;
            id /= 2;
        }

        SmallGraph v_cover_graph;
        for (uint32_t v : vertex_cover)
        {
          v_cover_graph.true_adj_list[v];
        }

        if (labelling_type() == Graph::LABELLED || labelling_type() == Graph::PARTIALLY_LABELLED)
        {
          v_cover_graph.labels.resize(query_graph.num_vertices());
        }
        for (const auto &[u, nbrs] : query_graph.true_adj_list)
        {
          for (uint32_t v : nbrs)
          {
            if (u > v) continue;
            bool e1 = (vertex_cover.find(u) != vertex_cover.end());
            bool e2 = (vertex_cover.find(v) != vertex_cover.end());
            if (e1 && e2)
            {
              v_cover_graph.true_adj_list[u].push_back(v);
              v_cover_graph.true_adj_list[v].push_back(u);

              if (labelling_type() == Graph::LABELLED || labelling_type() == Graph::PARTIALLY_LABELLED)
              {
                v_cover_graph.labels[u-1] = query_graph.labels[u-1];
                v_cover_graph.labels[v-1] = query_graph.labels[v-1];
              }
            }
            if (!e1 && !e2)
            {
              *status = -1;
              return v_cover_graph;
            }
          }
        }

        for (const auto &[u, nbrs] : query_graph.anti_adj_list)
        {
          for (uint32_t v : nbrs)
          {
            if (u > v) continue;

            // for safety: explicitly check to disallow anti-vertices
            // don't need to cover anti-vertex edges: hence we never select an anti-vertex
            if (is_anti_vertex(u) || is_anti_vertex(v)) continue;

            bool e1 = (vertex_cover.find(u) != vertex_cover.end());
            bool e2 = (vertex_cover.find(v) != vertex_cover.end());
            if (e1 && e2)
            {
              // it's an anti edge
              v_cover_graph.anti_adj_list[u].push_back(v);
              v_cover_graph.anti_adj_list[v].push_back(u);

              if (labelling_type() == Graph::LABELLED || labelling_type() == Graph::PARTIALLY_LABELLED)
              {
                v_cover_graph.labels[u-1] = query_graph.labels[u-1];
                v_cover_graph.labels[v-1] = query_graph.labels[v-1];
              }
            }
            if (!e1 && !e2)
            {
              *status = -1;
              return v_cover_graph;
            }
          }
        }

        if (!is_connected(vertex_cover, v_cover_graph.true_adj_list))
        {
          *status = -1;
        }

        return v_cover_graph;
      }

      bool is_valid_qseq(const std::vector<uint32_t> &v_list, std::vector<std::pair<uint32_t, uint32_t>> po)
      {
          for (auto po_pair : po)
          {
              bool matched = false;
              for (size_t i = 0; i < v_list.size(); i++)
              {
                  for (size_t j = i + 1; j < v_list.size(); j++)
                  {
                      if (po_pair.first == v_list[i] && po_pair.second == v_list[j])
                      {
                          matched = true;
                          break;
                      }
                  }
                  if (matched)
                      break;
              }
              if (!matched)
                  return false;
          }
          return true;
      }

      SmallGraph normalize_vseq(std::vector<uint32_t> vseq, const SmallGraph &patt)
      {
        std::map<uint32_t, uint32_t> v_map;
        uint32_t new_vid = 0;
        for (auto vertex_id : vseq)
        {
            v_map.insert({vertex_id, new_vid + 1});
            new_vid++;
        }
        SmallGraph n_vseq;
        n_vseq.labels.resize(new_vid);
        for (auto &v : vseq)
        {
          uint32_t current_vid = v_map.find(v)->second;
          for (size_t j = 0; j < patt.true_adj_list.at(v).size(); j++)
          {
            uint32_t old_nbr_vid = patt.true_adj_list.at(v)[j];
            uint32_t new_nbr_vid = v_map.find(old_nbr_vid)->second;

            if (labelling_type() == Graph::LABELLED || labelling_type() == Graph::PARTIALLY_LABELLED)
            {
              n_vseq.labels[new_nbr_vid-1] = patt.labels[old_nbr_vid-1];
            }

            n_vseq.true_adj_list[current_vid].push_back(new_nbr_vid);
          }

          if (patt.anti_adj_list.contains(v))
          {
            for (size_t j = 0; j < patt.anti_adj_list.at(v).size(); j++)
            {
              uint32_t old_nbr_vid = patt.anti_adj_list.at(v)[j];
              uint32_t new_nbr_vid = v_map.find(old_nbr_vid)->second;

              if (labelling_type() == Graph::LABELLED || labelling_type() == Graph::PARTIALLY_LABELLED)
              {
                n_vseq.labels[new_nbr_vid-1] = patt.labels[old_nbr_vid-1];
              }

              n_vseq.anti_adj_list[current_vid].push_back(new_nbr_vid);
            }
          }
        }
        return n_vseq;
      }

      bool is_same_vseq(SmallGraph &vs1, SmallGraph &vs2)
      {
          assert(vs1.true_adj_list.size() == vs2.true_adj_list.size());

          if ((labelling_type() == Graph::LABELLED || labelling_type() == Graph::PARTIALLY_LABELLED)
              && vs1.labels != vs2.labels)
          {
            return false;
          }

          for (auto &v : vs1.v_list())
          {
              if (vs2.true_adj_list[v].size() != vs1.true_adj_list[v].size())
              {
                  return false;
              }

              if (vs2.anti_adj_list[v].size() != vs1.anti_adj_list[v].size())
              {
                  return false;
              }

              sort(vs1.true_adj_list[v].begin(), vs1.true_adj_list[v].end());
              sort(vs2.true_adj_list[v].begin(), vs2.true_adj_list[v].end());

              for (size_t j = 0; j < vs2.true_adj_list[v].size(); j++)
              {
                  if (vs2.true_adj_list[v][j] != vs1.true_adj_list[v][j])
                      return false;
              }

              sort(vs1.anti_adj_list[v].begin(), vs1.anti_adj_list[v].end());
              sort(vs2.anti_adj_list[v].begin(), vs2.anti_adj_list[v].end());

              for (size_t j = 0; j < vs2.anti_adj_list[v].size(); j++)
              {
                  if (vs2.anti_adj_list[v][j] != vs1.anti_adj_list[v][j])
                      return false;
              }
          }

          return true;
      }

      std::vector<SmallGraph> remove_duplicate_vseq(std::vector<SmallGraph> vseq)
      {
          std::vector<SmallGraph> unique_vseq;
          for (size_t i = 0; i < vseq.size(); i++)
          {
              bool is_dup = false;
              for (size_t j = i + 1; j < vseq.size(); j++)
              {
                  if (is_same_vseq(vseq[i], vseq[j]))
                  {
                      is_dup = true;
                      break;
                  }
              }
              if (!is_dup)
              {
                  unique_vseq.push_back(vseq[i]);
              }
          }
          return unique_vseq;
      }

      uint32_t num_aut_sets() const
      {
        return nautsets;
      }

      std::vector<std::pair<uint32_t, uint32_t>> get_conditions()
      {
        std::vector<std::pair<uint32_t, uint32_t>> result;
        // XXX: and no anti-edges or vertices, and no labels
        if (is_clique() && labelling_type() == Graph::UNLABELLED)
        {
          for (uint32_t u = 1; u <= query_graph.num_vertices(); ++u)
          {
            for (uint32_t v = u+1; v <= query_graph.num_vertices(); ++v)
            {
              result.emplace_back(u, v);
            }
          }
        }
        else
        {
          bliss::Graph bliss_qg;
          uint32_t max_v = query_graph.num_vertices();

          std::vector<uint32_t> mapping(max_v);
          // construct bliss graph with vertices sorted by degree
          std::vector<uint32_t> degs(max_v);
          // regular vertex anti-edges should not
          for (const auto &[u, nbrs] : query_graph.true_adj_list)
          {
            degs[u-1] = nbrs.size();
          }

          std::vector<uint32_t> sorted_v(max_v);
          for (uint32_t i = 0; i < max_v; ++i)
          {
            sorted_v[i] = i;
          }
          std::sort(sorted_v.begin(), sorted_v.end(), [&degs](uint32_t a, uint32_t b) { return degs[a] > degs[b]; });

          for (uint32_t i = 0; i < max_v; ++i)
          {
            mapping[sorted_v[i]] = i;
          }

          if (labelling_type() == Graph::LABELLED || labelling_type() == Graph::PARTIALLY_LABELLED)
          {
            for (size_t i = 0; i < max_v; i++)
            {
                bliss_qg.add_vertex(query_graph.labels[sorted_v[i]]);
            }
          }
          else
          {
            for (size_t i = 0; i < max_v; i++)
            {
                bliss_qg.add_vertex(0);
            }
          }

          for (const auto &[u, nbrs] : query_graph.true_adj_list)
          {
            for (uint32_t v : nbrs)
            {
              if (u < v)
              {
                bliss_qg.add_edge(mapping[u - 1], mapping[v - 1]);
              }
            }
          }

          // give anti-vertices special label
          uint32_t vv = max_v;
          for (uint32_t av : anti_vertices)
          {
            bliss_qg.add_vertex(static_cast<uint32_t>(-2));
            for (uint32_t avn : query_graph.anti_adj_list.at(av))
            {
              bliss_qg.add_edge(vv, mapping[avn - 1]);
            }
            vv += 1;
          }

          auto conds = PO::findPOs(bliss_qg);

          for (const auto &cond : conds)
          {
            // make sure anti-vertex conditions are not used
            if (!is_anti_vertex(cond.first) && !is_anti_vertex(cond.second))
            {
              result.emplace_back(sorted_v[cond.first-1]+1,
                  sorted_v[cond.second-1]+1);
            }
            else
            {
              // anti-vertex should never be symmetric with regular vertices!
              assert(is_anti_vertex(cond.first) && is_anti_vertex(cond.second));
            }
          }

          auto autsets = PO::automorphicSets(bliss_qg);
          aut_map.resize(query_graph.num_vertices());
          for (const auto &set : autsets)
          {
            for (auto v : set)
            {
              if (v < max_v) // anti-vertices will only be automorphic to other anti-vertices
              {
                aut_map[sorted_v[v]] = nautsets;
              }
            }

            nautsets += 1;
          }

        }

        // turn off conditions:
        result = {};

        cond_adj.resize(query_graph.num_vertices());
        for (const auto &cond : result)
        {
          cond_adj[cond.first-1].push_back(cond.second-1);
          cond_adj[cond.second-1].push_back(cond.first-1);
        }

        return result;
      }

      void get_rbi_v()
      {
        uint32_t status = 0;
        uint32_t po_match_count = 0;
        if (is_clique() && labelling_type() == Graph::UNLABELLED)
        {
          rbi_v = get_mvcv(1, &status);
        }
        else
        {
          uint32_t v_count = query_graph.num_vertices();
          for (int32_t i = 1; i <= (1 << query_graph.num_vertices()) - 1; i++)
          {
            status = 0;
            SmallGraph vertex_cover = get_mvcv(i, &status);
            if (status == 0)
            {
              if (vertex_cover.num_vertices() == v_count)
              {
                uint32_t current_po_match_count = match_po(vertex_cover.v_list(), conditions);
                if (current_po_match_count > po_match_count)
                {
                  po_match_count = current_po_match_count;
                  rbi_v = vertex_cover;
                  v_count = vertex_cover.num_vertices();
                }
                else if (current_po_match_count == po_match_count)
                {
                  if (vertex_cover.num_true_edges() >= rbi_v.num_true_edges())
                  {
                    rbi_v = vertex_cover;
                    v_count = vertex_cover.num_vertices();
                  }
                }
              }
              else if (vertex_cover.num_vertices() < v_count)
              {
                rbi_v = vertex_cover;
                v_count = vertex_cover.num_vertices();
              }
            }
          }
        }
      }

      bool is_core_vertex(uint32_t v) const
      {
        return rbi_v.true_adj_list.contains(v);
      }

      void get_matching_groups()
      {
        // Sibling groups share the same candidate set.
        // Order groups are totally ordered subsets of sibling groups.
        // Non-core-independent vertices are non-core vertices independent of the
        // other non-core vertices (i.e. not in any order group). They may be still
        // dependent on core vertices.
        // So, we need to generate sibling groups, and for each one
        // generate order groups and non-core-independent vertices
        std::vector<std::vector<uint32_t>> cond_outadj(query_graph.num_vertices());
        for (const auto &cond : conditions)
        {
          cond_outadj[cond.first - 1].push_back(cond.second - 1);
        }

        // put the noncore vertices in their partial ordering
        // note that anti-vertices should not be present in sibling/order groups
        std::vector<bool> visited(query_graph.num_vertices(), false);
        std::vector<uint32_t> ordering;
        for (uint32_t nrv = 1; nrv <= query_graph.num_vertices(); ++nrv)
        {
          if (visited[nrv-1] || // visited
              is_core_vertex(nrv) || // core vertex
              is_anti_vertex(nrv)) // anti-vertex
          {
            continue;
          }

          std::queue<uint32_t> q;
          q.push(nrv-1);
          while (!q.empty())
          {
            uint32_t v = q.front();
            q.pop();
            ordering.push_back(v+1);

            for (uint32_t s : cond_outadj[v])
            {
              if (!visited[s] && !is_core_vertex(s+1))
              {
                visited[s] = true;
                q.push(s);
              }
            }
          }
        }

        // now we group them with their siblings
        std::vector<bool> visited2(ordering.size(), false);
        for (uint32_t i = 0; i < ordering.size(); ++i)
        {
          if (visited2[i]) continue;

          std::vector<uint32_t> group = {ordering[i]};
          for (uint32_t j = i+1; j < ordering.size(); ++j)
          {
            // have same candidate sets
            if (query_graph.true_adj_list.at(ordering[i]) == query_graph.true_adj_list.at(ordering[j]) &&
                query_graph.anti_adj_list.at(ordering[i]) == query_graph.anti_adj_list.at(ordering[j]))
            {
              visited2[j] = true;
              group.push_back(ordering[j]);
            }
          }

          sibling_groups.push_back(ordering[i]);

          // generate order groups
          // group are already partially ordered, just need to extract connected components
          // i.e., order groups are just indsets in a SiblingGroup's group
          std::vector<bool> visited(cond_adj.size(), false);
          for (uint32_t uu = 0; uu < group.size(); ++uu)
          {
            uint32_t u = group[uu] - 1;
            if (visited[u]) continue;
            visited[u] = true;
            std::vector<uint32_t> ordg = {u+1};

            // bfs from u to find connected component
            std::queue<uint32_t> q;
            q.push(u);
            while (!q.empty())
            {
              uint32_t v = q.front();
              q.pop();

              for (uint32_t s : cond_adj[v])
              {
                // s must be a member of the sibling group
                if (!visited[s] && utils::search(group, s+1))
                {
                  visited[s] = true;
                  q.push(s);
                  ordg.push_back(s+1);
                }
              }
            }
            order_groups.push_back(ordg);
            candidate_idxs.push_back(rbi_v.num_vertices() + sibling_groups.size()-1);
          }
        }

        // This drastically improves performance on 4-stars with certain labellings, when normally
        // the larger order group is scheduled first
        // Is this a general optimization? Why exactly does it matter?
        std::sort(order_groups.begin(), order_groups.end(), [](auto &a, auto &b) { return a.size() < b.size(); });
      }

      // connected components of graph given by conditions
      void get_indsets()
      {
        std::vector<bool> visited(query_graph.num_vertices(), false);
        for (uint32_t u = 0; u < query_graph.num_vertices(); ++u)
        {
          if (visited[u]) continue;
          visited[u] = true;
          uint32_t indset_idx = indsets.size();
          indsets.push_back({u+1});

          // bfs from u to find connected component
          std::queue<uint32_t> q;
          q.push(u);
          while (!q.empty())
          {
            uint32_t v = q.front();
            q.pop();

            for (uint32_t s : cond_adj[v])
            {
              if (!visited[s])
              {
                visited[s] = true;
                q.push(s);
                indsets[indset_idx].push_back(s+1);
              }
            }
          }
        }
      }

      void get_bounds()
      {
          // vertices whose intersections with core parents need to be discarded from noncore candidates
          std::vector<uint32_t> core_v(rbi_v.v_list());
          for (uint32_t i = 1; i <= query_graph.num_vertices(); ++i) {
            if (!is_core_vertex(i) && !is_anti_vertex(i))
            {
              ncore_vertices.push_back(i);
            }
          }

          // get bounds
          lower_bounds.resize(query_graph.num_vertices()+1);
          upper_bounds.resize(query_graph.num_vertices()+1);
          if (is_clique() && labelling_type() == Graph::UNLABELLED)
          {
            lower_bounds[query_graph.num_vertices()].push_back(core_v.back());
          }
          else
          {
            std::unordered_map<uint32_t, std::vector<uint32_t>> lower_adj;
            std::unordered_map<uint32_t, std::vector<uint32_t>> upper_adj;
            for (uint32_t j = 0; j < conditions.size(); ++j) {
              // things smaller
              lower_adj[conditions[j].second].push_back(conditions[j].first);
              // things larger
              upper_adj[conditions[j].first].push_back(conditions[j].second);
            }

            for (uint32_t i = 0; i < ncore_vertices.size(); ++i) {
              uint32_t v = ncore_vertices[i];

              // find transitive relations that tighten the bounds
              for (uint32_t u : lower_adj[v]) {
                // u < v, so for all x < u, x is superseded by u
                // and for all x > u, u is superseded by x if x < v
                // so just need to check if there are any x > u such that x < v
                bool minimal = true;
                for (uint32_t x : upper_adj[u]) {
                  if (std::find(lower_adj[v].begin(), lower_adj[v].end(), x) != lower_adj[v].end()) {
                    minimal = false;
                    break;
                  }
                }

                // only consider noncore vertices we match BEFORE v
                // we can't do anything about the ones we've yet to match
                auto is_core = is_core_vertex(u);
                if (minimal && (!is_core || u < v)) {
                  lower_bounds[v].push_back(u);
                }
              }

              for (uint32_t u : upper_adj[v]) {
                // u > v, so for all x > u, x is superseded by u
                // and for all x < u, u is superseded by x if x > v
                // so just need to check if there are any x < u such that x > v
                bool maximal = true;
                for (uint32_t x : lower_adj[u]) {
                  if (std::find(upper_adj[v].begin(), upper_adj[v].end(), x) != upper_adj[v].end()) {
                    maximal = false;
                    break;
                  }
                }

                auto is_core = is_core_vertex(u);
                if (maximal && (is_core || u < v)) {
                  upper_bounds[i].push_back(u);
                }
              }
            }
          }
      }

      void handle_star()
      {
        is_star = true;
        // unnecessary
        global_order = {1};

        uint32_t centre = *rbi_v.v_list().begin();
        qo_book.push_back({1});
        vgs.push_back(SmallGraph());
        vgs[0].true_adj_list.insert({1, {}});
        vgs[0].labels = {query_graph.labels[centre-1]};
        vmap = {{{}, {centre}}};
        get_bounds();
        get_indsets();
        get_matching_groups();
      }

      void build_rbi_graph()
      {
          get_rbi_v();
          std::vector<uint32_t> core_v(rbi_v.v_list());

          // special case for stars
          if (core_v.size() == 1) {
            handle_star();
            return;
          }

          get_bounds();
          get_indsets();
          get_matching_groups();

          std::vector<uint32_t> core_vmap;
          core_vmap.push_back(0);
          for (auto v : core_v)
          {
              core_vmap.push_back(v);
          }

          std::vector<std::pair<uint32_t, uint32_t>> core_po;
          for (const auto &po_pair: conditions)
          {
              uint32_t v1 = po_pair.first;
              uint32_t v2 = po_pair.second;
              if (utils::search(core_v, v1) && utils::search(core_v, v2))
              {
                for (uint32_t i = 1; i < core_vmap.size(); i++)
                {
                  if (v1 == core_vmap[i])
                  {
                    v1 = i;
                    break;
                  }
                }
                for (uint32_t i = 1; i < core_vmap.size(); i++)
                {
                  if (v2 == core_vmap[i])
                  {
                    v2 = i;
                    break;
                  }
                }
                core_po.push_back({v1,v2});
              }
          }

          rbi_v = normalize_vseq(core_v, rbi_v);

          std::vector<std::vector<uint32_t>> valid_qseq;
          std::vector<uint32_t> n_core_v(rbi_v.v_list());
          if (is_clique() && labelling_type() == Graph::UNLABELLED)
          {
            valid_qseq.push_back(n_core_v);
          }
          else
          {
            do
            {
                if (is_valid_qseq(n_core_v, core_po))
                    valid_qseq.push_back(n_core_v);
            } while (next_permutation(n_core_v.begin(), n_core_v.end()));
          }

          for (size_t i = 0; i < valid_qseq.size(); i++)
          {
              SmallGraph curr_vgs = normalize_vseq(valid_qseq[i], rbi_v);

              bool dup = false;
              for (size_t j = 0; j < vgs.size(); j++)
              {
                  SmallGraph past_vgs = vgs[j];
                  if (is_same_vseq(curr_vgs, past_vgs))
                  {
                      for (size_t k = 0; k < valid_qseq[i].size(); k++)
                          vmap[j][k + 1].push_back(core_vmap[valid_qseq[i][k]]);

                      qs[j].push_back(valid_qseq[i]);
                      dup = true;
                      break;
                  }
              }

              if (!dup)
              {
                  std::vector<std::vector<uint32_t>> curr_vmap(valid_qseq[i].size() + 1);
                  for (size_t k = 0; k < valid_qseq[i].size(); k++)
                      curr_vmap[k + 1].push_back(core_vmap[valid_qseq[i][k]]);

                  vmap.push_back(curr_vmap);
                  vgs.push_back(curr_vgs);
                  qs.push_back({valid_qseq[i]});
              }
          }

          global_order = rbi_v.v_list();
          build_qo_order();

          // make sure .at() never throws
          {
            for (auto &s : vgs)
            {
              for (const auto &lst : s.true_adj_list)
              {
                uint32_t v = lst.first;
                s.anti_adj_list[v];
              }
            }
          }
      }

      void get_qo(const SmallGraph &vgs, uint32_t source, uint32_t vgs_id, std::vector<bool> &visited)
      {
        qo_book[vgs_id].push_back(source);
        visited[source] = true;
        for (auto neighbour : vgs.true_adj_list.at(source))
        {
          if (!visited[neighbour])
          {
            get_qo(vgs,neighbour,vgs_id,visited);
          }
        }
      }

      void build_qo_order()
      {
          qo_book.resize(vgs.size());
          auto vgs_id = 0;
          for(const SmallGraph &vgs : this->vgs) {
              std::vector<bool> visited(vgs.num_vertices()+1, false);
              get_qo(vgs, global_order[vgs.num_vertices()-1], vgs_id, visited);
              vgs_id++;
          }
      }
  };

} // namespace Peregrine

namespace std
{

  template <>
  struct hash<Peregrine::SmallGraph>
  {
    // Note! this only works if you're comparing the true edges of graphs
    std::size_t operator()(const Peregrine::SmallGraph &k) const
    {
      return k.bliss_hash();
    }
  };
}

#endif
