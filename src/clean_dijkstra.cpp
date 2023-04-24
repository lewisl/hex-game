// ##########################################################################
// #             class Dijkstra
// # find minimum cost path, slightly adapted for playing Hex
// # initialized with the graph through which we find the shortest paths
// # inputs from class HexBoard make it possible to find a path of markers
// #    on the board from one player's starting edge to the finish edge
// ##########################################################################
template <typename T_data> // for data held in graph nodes like int, float, enum classes, string
class Dijkstra {
  public:
    Dijkstra(const Graph<T_data> &dijk_graph) : dijk_graph(dijk_graph) {} // initialize the graph to be analyzed.
    ~Dijkstra() = default;

    // fields
  private:
    set<int> path_nodes;
    unordered_map<int, int> path_costs; // dest. node -> cost to reach
    unordered_map<int, deque<int>> path_sequences; // dest. node -> path of nodes this node

  public:
    int start_node;
    const Graph<T_data> &dijk_graph; // need to have a graph to analyze. use reference
        // to avoid copying and we never change it.

    // methods
  public:
    // externally defined methods: effectively the real constructor
    void find_shortest_paths(int, int, bool);
    void find_shortest_paths(int, int, set<int>, bool);

    // print all shortest paths start_node to all other nodes
    friend ostream &operator<<(ostream &os, Dijkstra &dp)
    {
        for (int node : dp.path_nodes) {
            os << "||     Path to " << node << "     ||\n";
            os << "  cost: " << dp.path_costs[node] << "\n";
            os << "  sequence: "
               << "[ ";
            for (auto &seq : dp.path_sequences[node]) {
                os << seq << " ";
            }
            os << "]" << endl;
        }
        return os;
    }

    // methods to navigate resulting path_sequences
    bool path_sequence_exists(int node) { return path_sequences.find(node) != path_sequences.end() ? true : false; }

    // method for caller that doesn't prebuild candidate_nodes
    void find_shortest_paths(int start_here, T_data data_filter, bool verbose = false)
    {
        set<int> candidate_nodes;
        T_data node_val;   // = HexBoard::marker::empty;
        // candidates for the shortest paths must match the value in 'data_filter'
        for (auto node : dijk_graph.all_nodes) {
            node_val = dijk_graph.get_node_data(node);
            if (node_val == data_filter) {
                candidate_nodes.insert(node);
            }
        }

        find_shortest_paths(start_here, data_filter, candidate_nodes, false);
    }

    // method for caller that prebuilds appropriate candidate nodes
    void find_shortest_paths(int start_here, T_data data_filter, set<int> candidate_nodes, bool verbose = false)
    {

        int num_nodes = dijk_graph.count_nodes();

        // reserve memory for members and influence number of buckets
        // this improves performance and increases memory consumption, so good for n
        // < 1000
        path_costs.reserve(num_nodes);
        path_costs.max_load_factor(0.8);
        path_sequences.reserve(num_nodes);
        path_sequences.max_load_factor(0.8);

        start_node = start_here;

        // local variables
        const int inf = INT_MAX;
        int neighbor_node = 0;
        int current_node = start_node;
        int prev_node = 0;
        int min = inf;
        int tmp_cost = 0;
        T_data node_val;
        deque<int> tmpsequence;
        // set<int> candidate_nodes;   // initialized below with only valid nodes
        vector<Edge> neighbors; // end_node and cost for each neighbor node

        unordered_map<int, int> previous;
        previous.reserve(num_nodes);
        previous.max_load_factor(0.8);

        // candidates for the shortest paths must match the current player in
        // 'data_filter'
        for (auto node : candidate_nodes) {
            node_val = dijk_graph.get_node_data(node);
            if (node_val == data_filter) {
                // candidate_nodes.insert(node);
                path_costs[node] = inf; // initialize costs to inf for Dijkstra alg.
            }
        }

        path_costs[start_node] = 0;
        previous[start_node] = -1;

        // algorithm loop
        while (!(candidate_nodes.empty())) {
            // current_node begins as start_node

            if (verbose)
                cout << "\ncurrent_node at top of loop " << current_node << endl;

            if (dijk_graph.get_node_data(current_node) != data_filter)
                break;

            neighbors = dijk_graph.get_neighbors(current_node, data_filter,
                                                 path_nodes); //, path_nodes);  // vector<Edge>
            for (auto &neighbor : neighbors) { // neighbor is an Edge
                neighbor_node = neighbor.to_node;
                tmp_cost = path_costs[current_node] + neighbor.cost; // update path_costs for neighbors of current_node
                if (tmp_cost < path_costs[neighbor_node]) {
                    path_costs[neighbor_node] = tmp_cost;
                    previous[neighbor_node] = current_node;
                }
            }

            if (current_node == start_node && neighbors.empty())
                break; // start_node is detached from all others

            if (verbose) {
                //  for debugging shortest path algorithm
                cout << "**** STATUS ****\n";
                cout << "  current_node " << current_node << endl;
                cout << "  neighbors\n";
                cout << neighbors << endl;
                cout << "  path_nodes\n";
                cout << "    " << path_nodes << endl;
                cout << "  candidate_nodes\n";
                cout << "    " << candidate_nodes << endl;
                cout << "  previous\n";
                cout << previous << endl;
                cout << "  path cost\n";
                cout << path_costs << endl;
            }

            path_nodes.insert(current_node);
            candidate_nodes.erase(current_node); // we won't look at this node again

            // pick next current_node based on minimum distance, which will be a
            // neighbor whose cost has been updated from infinity to the network
            // edge cost
            min = inf;
            for (const int tmp_node : candidate_nodes) { // always O(n)
                if (path_costs[tmp_node] < min) {
                    min = path_costs[tmp_node];
                    current_node = tmp_node;
                }
            }

            if (min == inf) { // none of the remaining nodes are reachable
                break;
            }

            if (verbose)
                cout << "  current node at bottom: " << current_node << endl;
        }

        // build sequences by walking backwards from each dest. node to start_node
        for (const auto walk_node : path_nodes) {
            prev_node = walk_node;
            while (prev_node != start_node) {
                tmpsequence.push_front(prev_node); // it's a deque
                prev_node = previous[prev_node];
            };
            tmpsequence.push_front(prev_node);
            path_sequences[walk_node] = tmpsequence;

            tmpsequence.clear(); // IMPORTANT: clear before reusing to build a new
                // sequence!
        }

        if (verbose)
            cout << *this << endl; // prints the nodes and shortest path_sequences
                // to each node
    }
};
// end of class Dijkstra
// ##########################################################################
// #
// #
// #
// # end class Dijkstra
// #
// #
// #
// ##########################################################################

marker who_won_dijkstra()
{
    marker side = marker::empty;
    marker winner = marker::empty;
    int finish_hex;
    int start_hex;
    set<int> candidates;

    // use Dijkstra shortest path algorithm to find winner
    // test for side one victory
    // for each hex in the finish_border for side 1, find a path that ends in start border

    for (int finish_hex : finish_border[enum2int(marker::player1)]) {
        if (get_hex_marker(finish_hex) != marker::player1)
            continue;
        Dijkstra<marker> paths(hex_graph);

        copy_move_seq(move_seq[enum2int(marker::player1)], candidates);
        // must be a copy because we remove items from candidates AND we need to keep move_seq

        // find a path that starts from the finish border
        paths.find_shortest_paths(finish_hex, marker::player1, candidates);
        for (int start_hex : start_border[static_cast<int>(marker::player1)]) {
            if (paths.path_sequence_exists(start_hex)) { // to the starting border
                winner = marker::player1;
                return winner;
            }
        }
    }

    // test for side two victory
    for (int finish_hex : finish_border[enum2int(marker::player2)]) {
        if (get_hex_marker(finish_hex) != marker::player2)
            continue;
        Dijkstra<marker> paths(hex_graph);

        copy_move_seq(move_seq[enum2int(marker::player2)], candidates);

        paths.find_shortest_paths(finish_hex, marker::player2, candidates);
        for (int start_hex : start_border[enum2int(marker::player2)]) {
            if (paths.path_sequence_exists(start_hex)) {
                winner = marker::player2;
                return winner;
            }
        }
    }

    return winner; // will always be 0 or marker::empty
}