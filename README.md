# OR-Set CRDT Implementation in C++
State-based Observed-Remove Set with comprehensive benchmarks and test suite

## What is an OR-Set CRDT?

OR-Set stands for **Observed-Remove Set** - it's a conflict-free replicated data type that lets multiple replicas maintain a set of elements without coordination. The key insight: concurrent adds win over removes.

### Working Intuition

**State:**
- Internally, the set stores pairs `(element, tag)`.
- Each tag is unique per add operation, e.g., `(replica_id, local_counter)`.
- This means the same element can have multiple tags if it's added multiple times.

**Operations:**

**add(x):**
- Increment `local_counter`.
- Create a new unique tag `t = (replica_id, local_counter)`.
- Insert `(x, t)` into the internal set.
- Broadcast "add element with tag" to other replicas.

**remove(x):**
- Look at the current local state.
- Collect all tags `T_x = { t | (x, t) is in the internal set }`.
- Remove all pairs `(x, t)` for `t` in `T_x` from the internal set.
- Broadcast "remove element with tags_to_remove" to other replicas.
- Note: remove only touches tags it can currently see.

**contains(x):**
- Return true if there exists at least one pair `(x, t)` in the internal set.
- Optimized with an element cache for O(1) lookup.

**merge(other):**
- For a simple state-based OR-Set:
  - Internal set := union of our internal set and other's internal set.
- Because we only ever *add* tags in merge (and never mutate them), merge is commutative, associative, and idempotent.

### Example with Two Replicas A and B

**Initial:**
```
A: {}
B: {}
```

**1) A does add("apple"):**
```
A.counter = 1
tag = (A,1)
A.state = { ("apple", (A,1)) }
```

**2) B does add("apple"):**
```
B.counter = 1
tag = (B,1)
B.state = { ("apple", (B,1)) }
```

**3) A and B sync (merge via union):**
```
Both end up with:
  { ("apple", (A,1)), ("apple", (B,1)) }
elements() = {"apple"}
```

**4) A does remove("apple"):**
```
Locally A sees tags for "apple": {(A,1), (B,1)}
T_apple = {(A,1), (B,1)}
A removes both pairs:
  A.state = {}
A broadcasts "remove apple with tags {(A,1),(B,1)}".
```

**5) Concurrently, B does add("apple") again:**
```
B.counter = 2
tag = (B,2)
B.state (before receiving A's remove) =
    { ("apple", (A,1)), ("apple", (B,1)), ("apple", (B,2)) }
```

**6) Now B receives A's remove message:**
```
Remove all ("apple", t) where t in {(A,1), (B,1)}
B.state becomes:
    { ("apple", (B,2)) }
```

**7) Eventually A and B sync again (merge via union):**
```
A merges with B and also ends up with:
    { ("apple", (B,2)) }
elements() = {"apple"}
```

### Key Points to Understand

- **Each add creates a fresh tag; tags are never reused.**
- **remove(x) only removes the tags for x that existed at that replica when remove() ran.**
- **Concurrent adds that create new tags are not affected by earlier removes that never saw them.** (This is the add-wins semantics)
- **merge is just a union of (element, tag) pairs, which makes sync order irrelevant.**

## Implementation Details

This implementation includes:
- **ORSet class** with full CRDT semantics
- **Element cache** using `unordered_set` for O(1) contains operations
- **Tag-based element tracking** for proper conflict resolution
- **State-based replication** via merge operation

## Features

- Add-wins conflict resolution
- Commutative, associative, and idempotent merge
- O(1) contains check with element caching
- Support for multiple replicas
- Comprehensive test suite
- Performance benchmarks

## Building and Running

Compile the main demo:
```bash
g++ -std=c++17 -o crdt crdt.cpp
./crdt
```

Compile and run the benchmark suite:
```bash
g++ -std=c++17 -o crdt_benchmark crdt_benchmark.cpp
./crdt_benchmark
```

## Test Suite

The benchmark suite includes:
- Basic operations tests (add, remove, contains)
- Concurrent operations tests (add-wins semantics)
- Merge properties tests (idempotency, commutativity)
- Complex multi-replica scenarios

## Benchmarks

Performance benchmarks covering:
- Add operations (100 to 100K elements)
- Contains lookups (100 to 100K operations)
- Merge operations (100 to 50K elements)
- Remove operations (100 to 50K elements)
- Memory usage analysis

## Files

- `crdt.h` - Header file with ORSet class definition
- `crdt.cpp` - Main demo with detailed documentation
- `crdt_benchmark.cpp` - Comprehensive test and benchmark suite
- `crdt_benchmark_results.csv` - Benchmark results output

