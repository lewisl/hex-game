// ##########################################################################
// #             Definition/Declaration of Class Graph
// ##########################################################################

#ifndef GRAPH_H
#define GRAPH_H

#include <deque> // sequence of nodes in a path between start and destination
#include <fstream> // to write graph to file and read graph from file
#include <iostream>
#include <random>
#include <sstream> // to use stringstream to parse inputs from file
#include <string>
#include <unordered_map> // container for definition of Graph
#include <vector>

using namespace std;

#include "helpers.h"

/* 
##########################################################################
#                            class Graph
#  graph: data structure holding nodes and their edge
#  node_data: data structure for holding data value at each node
#  load_graph_from_file: method to define graph representation
#  this class is not really bound to hex in any way and can be used
#    for other graph applications.
##########################################################################
*/
template <typename T_data> // T_data can be various primitive data types
class Graph {
  public:
    Graph<T_data>() = default;
    ~Graph<T_data>() = default;

    vector<T_data> node_data; // holds Data values of all nodes

    // holds an edge for a starting node: to node, cost to the neighbor
    // doesn't include starting node because that is part of the graph container
    // all of the edges of a node are held in a vector of Edges
    struct Edge {
        int to_node; // use linear index to hexboard
        int cost; // default to cost=1 => should NOT change this when creating edges

        Edge(int to_node = 0, int cost = 1) : to_node(to_node), cost(cost) {}
    };

  private:
    unordered_map<int, vector<Edge>> graph;

  public:
    void set_storage(int size)
    {
        graph.reserve(size);
        graph.max_load_factor(0.8);
        node_data.reserve(size);
    }

    // output an Edge in an output stream
    friend ostream &operator<<(ostream &os, const Edge &e)
    {
        os << "  to: " << e.to_node << " cost: " << e.cost << endl;
        return os;
    }

    // output a vector of edges: used in the graph definition for each node
    friend ostream &operator<<(ostream &os, const vector<Edge> &ve)
    {
        for (auto const &e : ve) {
            os << e << endl;
        }
        return os;
    }

    int count_nodes() const { return graph.size(); }

    void initialize_data(const T_data val, const int size) { node_data.insert(node_data.begin(), size, val); };

    void set_node_data(const T_data val, const int idx) { node_data[idx] = val; }

    T_data get_node_data(int idx) const { return node_data[idx]; }

    // get the neighbors of a node as a vector of edges
    const vector<Edge> get_neighbors(const int current_node) const
    {
        return graph.at(current_node); // the value type of graph is
            // vector<Edge> and holds the neighbors
    }

    // get the neighbors that match the select data--the player whose marker is
    // there
    vector<Edge> get_neighbors(const int current_node, const T_data data_filter) const
    {
        vector<Edge> ve;
        for (const auto &e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter) {
                ve.push_back(e);
            }
        }
        return ve;
    }

    // get the neighbors that match the select data and are not in the exclude
    // set, deque or vector
    template <typename Container>
    vector<Edge> get_neighbors(const int current_node, const T_data data_filter, const Container &exclude_set) const
    {
        vector<Edge> vr;
        for (const auto &e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter && !is_in(e.to_node, exclude_set))
                vr.push_back(e);
        }
        return vr; // in c++14 this returns an rvalue reference so the caller
            // moves the returned value to a vector
    }

    // get the neighbor_nodes as a vector of nodes instead of the edges
    vector<int> get_neighbor_nodes(const int current_node, const T_data data_filter) const
    {
        vector<int> neighbor_nodes;
        for (const auto &e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter)
                neighbor_nodes.push_back(e.to_node);
        }
        return neighbor_nodes;
    }

    // get the neighbor_nodes that match the filter while excluding a set or vector of nodes
    template <typename Container>
    vector<int> get_neighbor_nodes(const int current_node, const T_data data_filter, const Container &exclude) const
    {
        vector<int> neighbor_nodes;
        for (const auto &e : graph.at(current_node)) {
            if (node_data[e.to_node] == data_filter && !is_in(e.to_node, exclude))
                neighbor_nodes.push_back(e.to_node);
        }
        return neighbor_nodes;
    }

    void create_edge_container(const int node) // empty Edge container
    {
        graph.emplace(node, vector<Edge>{});
    }

    // with to_node and cost
    void add_edge(const int node, const int y, const int cost = 1, const bool bidirectional = false)
    {
        if (graph.find(node) != graph.cend()) {
            if (graph.find(y) != graph.cend()) {
                for (auto &edge : graph[node]) { // iterate over edges of node
                    if (y == edge.to_node) {
                        return; // to_node y already exists as an edge of node
                    }
                }
                graph[node].push_back(Edge(y, cost));
                if (bidirectional)
                    graph[y].push_back(Edge(node, cost));
            }
            else {
                cout << "node " << y << " not found in graph.";
            }
        }
        else {
            cout << "node " << node << " not found in graph.";
        }
    }

    /** 
    display_graph
    Print graph to an output stream ot.
    This is the table of nodes and edges in the graph, not a picture of the
    graph. It is identical to the format used by load_graph_from_file. You can 
    pass an fstream as the output stream to save the graph in a text file.
    cout is the default value of ot to print to console.
    */
    void display_graph(ostream &ot = cout, bool to_file = false) const
    {
        vector<int> nodes_sorted(graph.size());

        // copy and sort the keys in graph
        std::transform(graph.begin(), graph.end(), nodes_sorted.begin(), [](const auto &pair) { return pair.first; });
        std::sort(nodes_sorted.begin(), nodes_sorted.end());

        ot << "\nsize " << graph.size() << "\n";
        for (const auto node : nodes_sorted) { // just an int node
            ot << "node " << node << endl;
            ot << "    data " << get_node_data(node) << endl;
            auto &ve = graph.at(node); // vector<Edge>
            for (const auto &edge : ve) { // Edge in vector<Edge>
                ot << "    "
                   << "edge " << edge.to_node << " " << edge.cost << endl;
            }
        }
    }

    // note: we don't use this for the monte carlo simulation, but it's good for testing
    /*** load_graph_from_file
        Read a graph file to initialize a graph using the format of this example:
        size 4        // optional: if used, will check if it matches actual number of input nodes
        node 0        // node must be positive integer; not required to be consecutive
            data 0    // data value at this position (can be used to represent hex markers)
            edge 2 3  // edge is to_node, cost. No error checking so to_node must exist
        node 1
            data 1
            edge 1 4
            edge 3 5
        node 2
            data 1
            edge 1 4
            edge 3 5
        node 3
            data 1
            edge 1 4
            edge 3 5

        Note: in this format, edges are assumed to be directional. If you want
        bidirectional (aka, non-directional edges that go both ways) then you have to include 2
        reciprocal edges.
    */
    void load_graph_from_file(string filename)
    {
        // prepare input file
        ifstream infile;
        infile.open(filename); // open a file to perform read operation using file object
        if (!(infile.is_open())) {
            cout << "Error opening file: " << filename << " Terminating.\n";
            exit(-1);
        }

        // read the file line by line and create graph
        string linestr, leader;
        size_t node_id, to_node;
        int cost;
        int tmp_size = 0; // only used locally here
        int input_data; // as read from text file
        // T_data data;

        // loads data into graph and positions
        while (getline(infile, linestr)) {
            stringstream ss{linestr};
            ss >> leader; // first word or text up to first white space

            if (leader == "size") { // define size constants and reserve memory
                ss >> tmp_size;

                // reserve storage
                set_storage(tmp_size);
                // graph.reserve(tmp_size);
                // graph.max_load_factor(0.8);
                // node_data.reserve(tmp_size);
            }
            else if (leader == "node") {
                ss >> node_id;
                graph[node_id] = vector<Edge>{}; // create node with empty vector of Edges
            }
            else if (leader == "edge") {
                ss >> to_node >> cost;
                graph[node_id].push_back(Edge(to_node, cost)); // push Edge to current node_id
            }
            else if (leader == "data") {
                ss >> input_data;
                set_node_data(static_cast<T_data>(input_data), node_id);
            }
        }
        if (tmp_size != 0) {
            if (tmp_size != graph.size())
                cout << "Error: number of nodes in file " << graph.size() << " does not match size input " << tmp_size
                     << endl;
        }
    }
}; // end class Graph
// ##########################################################################
// #
// #
// #
// # end class Graph
// #
// #
// #
// ##########################################################################

#endif