// crdt_benchmark.cpp - Comprehensive testing and benchmarking for OR-Set CRDT

#include "crdt.h"
#include <chrono>

using namespace std;
using namespace std::chrono;

// ============= TEST SUITE =============

class TestRunner {
private:
    int passed = 0;
    int failed = 0;

public:
    void assert_true(bool condition, const string& test_name) {
        if (condition) {
            cout << "[PASS] " << test_name << endl;
            passed++;
        } else {
            cout << "[FAIL] " << test_name << endl;
            failed++;
        }
    }

    void print_summary() {
        cout << "\n========== TEST SUMMARY ==========\n";
        cout << "Passed: " << passed << endl;
        cout << "Failed: " << failed << endl;
        cout << "Total:  " << (passed + failed) << endl;
        cout << "==================================\n\n";
    }
};

void test_basic_operations(TestRunner& runner) {
    cout << "\n=== Basic Operations Tests ===\n";

    ORSet set("test");

    // Test add
    set.add("apple");
    runner.assert_true(set.contains("apple"), "Add single element");
    runner.assert_true(set.size() == 1, "Size after add");

    // Test multiple adds
    set.add("banana");
    set.add("cherry");
    runner.assert_true(set.size() == 3, "Multiple adds");

    // Test remove
    set.remove("banana");
    runner.assert_true(!set.contains("banana"), "Remove element");
    runner.assert_true(set.size() == 2, "Size after remove");

    // Test contains
    runner.assert_true(set.contains("apple"), "Contains existing");
    runner.assert_true(!set.contains("xyz"), "Contains non-existing");
}

void test_concurrent_operations(TestRunner& runner) {
    cout << "\n=== Concurrent Operations Tests ===\n";

    ORSet A("A"), B("B");

    // Concurrent adds of same element
    A.add("apple");
    B.add("apple");
    A.merge(B);
    B.merge(A);

    runner.assert_true(A.contains("apple"), "Concurrent add - A");
    runner.assert_true(B.contains("apple"), "Concurrent add - B");
    runner.assert_true(A.size() == 1, "No duplicates after merge");

    // Concurrent add/remove (add-wins)
    A.remove("apple");
    B.add("apple");
    A.merge(B);
    B.merge(A);

    runner.assert_true(A.contains("apple"), "Add-wins semantics - A");
    runner.assert_true(B.contains("apple"), "Add-wins semantics - B");
}

void test_merge_idempotency(TestRunner& runner) {
    cout << "\n=== Merge Properties Tests ===\n";

    ORSet A("A"), B("B");
    A.add("x");
    B.add("y");

    // Test idempotency
    size_t size_before = A.size();
    A.merge(B);
    size_t size_after_first = A.size();
    A.merge(B);  // Merge again
    size_t size_after_second = A.size();

    runner.assert_true(size_after_first == size_after_second, "Merge is idempotent");

    // Test commutativity
    ORSet C("C"), D("D");
    C.add("a");
    D.add("b");

    ORSet C_copy = C;
    C.merge(D);
    D.merge(C_copy);

    runner.assert_true(C.size() == D.size(), "Merge is commutative");
}

void test_complex_scenario(TestRunner& runner) {
    cout << "\n=== Complex Multi-Replica Scenario ===\n";

    ORSet A("A"), B("B"), C("C");

    // Replica A operations
    A.add("item1");
    A.add("item2");

    // Replica B operations
    B.add("item2");
    B.add("item3");

    // Replica C operations
    C.add("item1");
    C.add("item3");

    // Partial sync: A <-> B
    A.merge(B);
    B.merge(A);

    // A removes item2
    A.remove("item2");

    // C adds item4
    C.add("item4");

    // Full sync
    A.merge(C);
    B.merge(C);
    C.merge(A);
    C.merge(B);
    A.merge(C);
    B.merge(A);

    runner.assert_true(A.size() == B.size() && B.size() == C.size(), "All replicas converged");
    runner.assert_true(!A.contains("item2"), "Removed item absent");
    runner.assert_true(A.contains("item4"), "New item present");
}

// ============= BENCHMARKS =============

struct BenchmarkResult {
    string name;
    double avg_time_ms;
    size_t operations;
    double ops_per_sec;
};

void benchmark_add_operations(vector<BenchmarkResult>& results) {
    cout << "\n=== Benchmarking Add Operations ===\n";

    vector<int> sizes = {100, 1000, 10000, 100000};

    for (int n : sizes) {
        ORSet set("bench");

        auto start = high_resolution_clock::now();

        for (int i = 0; i < n; i++) {
            set.add("element_" + to_string(i));
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        double time_ms = duration.count() / 1000.0;
        double ops_per_sec = (n / time_ms) * 1000.0;

        BenchmarkResult result{
            "Add " + to_string(n) + " elements",
            time_ms,
            (size_t)n,
            ops_per_sec
        };
        results.push_back(result);

        cout << result.name << ": " << time_ms << " ms ("
             << ops_per_sec << " ops/sec)" << endl;
    }
}

void benchmark_contains_operations(vector<BenchmarkResult>& results) {
    cout << "\n=== Benchmarking Contains Operations ===\n";

    vector<int> sizes = {100, 1000, 10000, 100000};

    for (int n : sizes) {
        ORSet set("bench");

        // Populate set
        for (int i = 0; i < n; i++) {
            set.add("element_" + to_string(i));
        }

        auto start = high_resolution_clock::now();

        // Test contains
        for (int i = 0; i < n; i++) {
            set.contains("element_" + to_string(i));
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        double time_ms = duration.count() / 1000.0;
        double ops_per_sec = (n / time_ms) * 1000.0;

        BenchmarkResult result{
            "Contains " + to_string(n) + " lookups",
            time_ms,
            (size_t)n,
            ops_per_sec
        };
        results.push_back(result);

        cout << result.name << ": " << time_ms << " ms ("
             << ops_per_sec << " ops/sec)" << endl;
    }
}

void benchmark_merge_operations(vector<BenchmarkResult>& results) {
    cout << "\n=== Benchmarking Merge Operations ===\n";

    vector<int> sizes = {100, 1000, 10000, 50000};

    for (int n : sizes) {
        ORSet A("A"), B("B");

        // Populate both sets
        for (int i = 0; i < n; i++) {
            A.add("element_" + to_string(i));
            B.add("element_" + to_string(i + n/2)); // 50% overlap
        }

        auto start = high_resolution_clock::now();
        A.merge(B);
        auto end = high_resolution_clock::now();

        auto duration = duration_cast<microseconds>(end - start);
        double time_ms = duration.count() / 1000.0;

        BenchmarkResult result{
            "Merge sets of " + to_string(n) + " elements",
            time_ms,
            (size_t)n,
            0
        };
        results.push_back(result);

        cout << result.name << ": " << time_ms << " ms" << endl;
    }
}

void benchmark_remove_operations(vector<BenchmarkResult>& results) {
    cout << "\n=== Benchmarking Remove Operations ===\n";

    vector<int> sizes = {100, 1000, 10000, 50000};

    for (int n : sizes) {
        ORSet set("bench");

        // Populate set
        for (int i = 0; i < n; i++) {
            set.add("element_" + to_string(i));
        }

        auto start = high_resolution_clock::now();

        // Remove all elements
        for (int i = 0; i < n; i++) {
            set.remove("element_" + to_string(i));
        }

        auto end = high_resolution_clock::now();
        auto duration = duration_cast<microseconds>(end - start);

        double time_ms = duration.count() / 1000.0;
        double ops_per_sec = (n / time_ms) * 1000.0;

        BenchmarkResult result{
            "Remove " + to_string(n) + " elements",
            time_ms,
            (size_t)n,
            ops_per_sec
        };
        results.push_back(result);

        cout << result.name << ": " << time_ms << " ms ("
             << ops_per_sec << " ops/sec)" << endl;
    }
}

void benchmark_memory_usage() {
    cout << "\n=== Memory Usage Analysis ===\n";

    vector<int> sizes = {1000, 10000, 100000};

    for (int n : sizes) {
        ORSet set("bench");

        for (int i = 0; i < n; i++) {
            set.add("element_" + to_string(i));
        }

        // Approximate memory usage
        size_t internal_pairs = set.internal_size();
        size_t cached_elements = set.size();

        // Rough estimate: each pair ~(string + Tag) = ~50 bytes avg
        // each cached string ~30 bytes avg
        size_t estimated_memory = (internal_pairs * 50) + (cached_elements * 30);

        cout << "Set with " << n << " elements:\n";
        cout << "  Internal pairs: " << internal_pairs << "\n";
        cout << "  Unique elements: " << cached_elements << "\n";
        cout << "  Est. memory: " << (estimated_memory / 1024.0) << " KB\n";
    }
}

void save_results_to_file(const vector<BenchmarkResult>& results) {
    ofstream out("crdt_benchmark_results.csv");
    out << "Benchmark,Time(ms),Operations,Ops/Sec\n";

    for (const auto& r : results) {
        out << r.name << "," << r.avg_time_ms << ","
            << r.operations << "," << r.ops_per_sec << "\n";
    }

    out.close();
    cout << "\n[INFO] Results saved to crdt_benchmark_results.csv\n";
}

int main() {
    cout << "========================================\n";
    cout << "  OR-Set CRDT Test & Benchmark Suite  \n";
    cout << "========================================\n";

    TestRunner runner;

    // Run tests
    test_basic_operations(runner);
    test_concurrent_operations(runner);
    test_merge_idempotency(runner);
    test_complex_scenario(runner);
    runner.print_summary();

    // Run benchmarks
    vector<BenchmarkResult> results;

    benchmark_add_operations(results);
    benchmark_contains_operations(results);
    benchmark_merge_operations(results);
    benchmark_remove_operations(results);
    benchmark_memory_usage();

    // Save results
    save_results_to_file(results);

    cout << "\n========================================\n";
    cout << "  All tests and benchmarks completed!  \n";
    cout << "========================================\n";

    return 0;
}