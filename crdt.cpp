#include "crdt.h"

// OR-Set (Observed-Remove Set) - working intuition

// State:
//   - Internally, the set stores pairs (element, tag).
//   - tag is unique per add, e.g. (replica_id, local_counter).

// Operations:
//   add(x):
//     - Increment local_counter.
//     - Create a new unique tag t = (replica_id, local_counter).
//     - Insert (x, t) into the internal set.
//     - Broadcast "add element with tag" to other replicas.

//   remove(x):
//     - Look at the current local state.
//     - Collect all tags T_x = { t | (x, t) is in the internal set }.
//     - Remove all pairs (x, t) for t in T_x from the internal set.
//     - Broadcast "remove element with tags_to_remove" to other replicas.
//     - Note: remove only touches tags it can currently see.

//   contains(x):
//     - Return true if there exists at least one pair (x, t) in the internal set.

//   elements():
//     - Return the set of all distinct x such that there exists (x, t) in the internal set.

//   merge(other):
//     - For a simple state-based OR-Set:
//       - Internal set := union of our internal set and other's internal set.
//     - Because we only ever *add* tags in merge (and never mutate them),
//       merge is commutative, associative, and idempotent.

// Example with two replicas A and B:

// Initial:
//   A: {}
//   B: {}

// 1) A does add("apple"):
//    - A.counter = 1
//    - tag = (A,1)
//    - A.state = { ("apple", (A,1)) }

// 2) B does add("apple"):
//    - B.counter = 1
//    - tag = (B,1)
//    - B.state = { ("apple", (B,1)) }

// 3) A and B sync (merge via union):
//    - Both end up with:
//      { ("apple", (A,1)), ("apple", (B,1)) }
//    - elements() = {"apple"}

// 4) A does remove("apple"):
//    - Locally A sees tags for "apple": {(A,1), (B,1)}
//    - T_apple = {(A,1), (B,1)}
//    - A removes both pairs:
//      A.state = {}
//    - A broadcasts "remove apple with tags {(A,1),(B,1)}".

// 5) Concurrently, B does add("apple") again:
//    - B.counter = 2
//    - tag = (B,2)
//    - B.state (before receiving A's remove) =
//        { ("apple", (A,1)), ("apple", (B,1)), ("apple", (B,2)) }

// 6) Now B receives A's remove message:
//    - Remove all ("apple", t) where t in {(A,1), (B,1)}
//    - B.state becomes:
//        { ("apple", (B,2)) }

// 7) Eventually A and B sync again (merge via union):
//    - A merges with B and also ends up with:
//        { ("apple", (B,2)) }
//    - elements() = {"apple"}

// Key points to understand while coding:
//   - Each add creates a fresh tag; tags are never reused.
//   - remove(x) only removes the tags for x that existed *at that replica* when remove() ran.
//   - Concurrent adds that create new tags are not affected by earlier removes that never saw them.
//   - merge is just a union of (element, tag) pairs, which makes sync order irrelevant.

int main() {
    ORSet A("A");
    ORSet B("B");

    A.add("apple");
    B.add("apple");

    A.merge(B);
    B.merge(A);

    cout << "After first merge, A contains apple: " << A.contains("apple") << endl;
    cout << "After first merge, B contains apple: " << B.contains("apple") << endl;

    A.remove("apple");
    B.add("apple");

    B.merge(A);
    A.merge(B);

    cout << "After second merge, A contains apple: " << A.contains("apple") << endl;
    cout << "After second merge, B contains apple: " << B.contains("apple") << endl;

    return 0;
}