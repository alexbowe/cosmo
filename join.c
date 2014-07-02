#include "transform.h"
#include "io.h"
#include "sort.h"
#include "join.h"

// Remove rightmost nt (2 bits)
#define get_left_64(x)  ((x) >> 2)
// Remove (leftmost) kth nt (2 bits)
#define get_right_64(x, k) (((x) << (66 - (k)*2)) >> (66 - (k)*2))

// NOTE: This part might be possible with a counting approach (since we need the result to have the same number of symbols in each column)
// similar to radix sorting, but maintain the counts for each column
// then use an in place radix sort for the first part (where we make tables a and b)
// Already sorted on previous character, so use count for 0th col on symbol from 1th col to find range to match to
// Haven't worked it out, but seems like id have to end up sorting it again, so probably faster to use two tables instead
// especially since they are both a product of a single radix sort

size_t count_incoming_dummy_edges_64(uint64_t * table_a, uint64_t * table_b, size_t num_records, uint32_t k) {
  #define get_a(i) (block_reverse_64(get_left_64(table_a[(i)])))
  #define get_b(i) (block_reverse_64(get_right_64(table_b[(i)], k)))
  size_t count = 0;
  size_t a_idx = 0, b_idx = 0;
  uint64_t x = 0, y = 0, x_prev = 0, y_prev = 0;
  //char buf[k+1];

  if (num_records == 0) return 0;

  x = get_a(a_idx);
  y = get_b(b_idx);
  // nothing special about the NOT operation here, just need a guaranteed different value to start with
  x_prev = ~x;
  y_prev = ~y;

  while (a_idx < num_records && b_idx < num_records) {
    // B - A: These y-nodes from table B will require outgoing dummy edges
    while (b_idx < num_records && y < x) {
      // add y to result
      if (++b_idx >= num_records) break;
      y = get_b(b_idx);
    }

    // A - B: These x-nodes from table A will require incoming dummy edges
    while (a_idx < num_records && y > x) {
      // add x to result
      if (x != x_prev) {
        count++;
      }
      if (++a_idx >= num_records) break;
      x_prev = x;
      x = get_a(a_idx);
    }

    // These are the nodes that don't need dummy edges
    while (a_idx < num_records && b_idx < num_records && y == x) {
      x_prev = x;
      y_prev = y;

      // Scan to next non-equal (outputing all table entries?)
      while (a_idx < num_records && x == x_prev) {
        if (++a_idx >= num_records) break;
        x_prev = x;
        x = get_a(a_idx);
      }

      // Scan to next non-equal
      while (b_idx < num_records && y == y_prev) {
        if (++b_idx >= num_records) break;
        y_prev = y;
        y = get_b(b_idx);
      }
    }
  }
  return count;
}

void get_incoming_dummy_edges_64(uint64_t * table_a, uint64_t * table_b, size_t num_records, uint32_t k, uint64_t * incoming_dummies, size_t num_incoming_dummies) {
  #define get_a(i) (block_reverse_64(get_left_64(table_a[(i)])))
  #define get_b(i) (block_reverse_64(get_right_64(table_b[(i)], k)))
  size_t next = 0;
  size_t a_idx = 0, b_idx = 0;
  uint64_t x = 0, y = 0, x_prev = 0, y_prev = 0;

  if (num_records == 0 || num_incoming_dummies == 0) return;

  x = get_a(a_idx);
  y = get_b(b_idx);
  // nothing special about the NOT operation here, just need a guaranteed different value to start with
  x_prev = ~x;
  y_prev = ~y;

  while (a_idx < num_records && b_idx < num_records) {
    // B - A: These y-nodes from table B will require outgoing dummy edges
    while (b_idx < num_records && y < x) {
      // add y to result
      if (y != y_prev) {
        // should probably use the already fetched y
      }
      if (++b_idx >= num_records) break;
      y_prev = y;
      y = get_b(b_idx);
    }

    // A - B: These x-nodes from table A will require incoming dummy edges
    while (a_idx < num_records && y > x) {
      // add x to result
      if (x != x_prev) {
        // make this already fetched, and x calculated from it
        uint64_t temp = table_a[a_idx];
        temp >>= 2;
        incoming_dummies[next++] = block_reverse_64(temp);
        if (next == num_incoming_dummies) return;
      }
      if (++a_idx >= num_records) break;
      x_prev = x;
      x = get_a(a_idx);
    }

    // These are the nodes that don't need dummy edges
    while (a_idx < num_records && b_idx < num_records && y == x) {
      x_prev = x;
      y_prev = y;

      // Scan to next non-equal (outputing all table entries?)
      while (a_idx < num_records && x == x_prev) {
        if (++a_idx >= num_records) break;
        x_prev = x;
        x = get_a(a_idx);
      }

      // Scan to next non-equal
      while (b_idx < num_records && y == y_prev) {
        if (++b_idx >= num_records) break;
        y_prev = y;
        y = get_b(b_idx);
      }
    }
  }
  return;
}

void prepare_k_values(unsigned char * k_values, size_t num_dummies, uint32_t k);
void prepare_k_values(unsigned char * k_values, size_t num_dummies, uint32_t k) {
  // first num_dummies are k, then it is k-1 down to 1 num_dummies times
  memset(k_values, k, num_dummies);
  unsigned char * new_k_values = k_values + num_dummies;
  // TODO: check this loop
  for (size_t i = 0; i < num_dummies * (k-1); i++) {
    new_k_values[i] = k - (i % (k-1)) - 1;
  }
}

void generate_dummies_64(uint64_t dummy_node, uint64_t * output, uint32_t k);
void generate_dummies_64(uint64_t dummy_node, uint64_t * output, uint32_t k) {
  for (size_t i = 0; i < k-1; i++) {
    // shift the width of 1 nucleotide. At this stage the kmers should be represented as reversed
    output[i] = (dummy_node <<= 2);
  }
}

void prepare_incoming_dummy_edges_64(uint64_t * dummy_nodes, unsigned char * k_values, size_t num_dummies, uint32_t k) {
  // should be k * num_dummies space allocated for dummy_nodes and k_values
  // loop over each dummy edge, and have a pointer for next, loop over k, write k and edge >> 2
  uint64_t * output = dummy_nodes + num_dummies;
  for (size_t i = 0; i < num_dummies; i++) {
    generate_dummies_64(dummy_nodes[i], output + i * (k-1), k);
  }
  prepare_k_values(k_values, num_dummies, k);
  // SORT - quicksort since it is probably a small vector and I don't want to double the space even further
  // but since we have two arrays I have to write my own instead of just a comparator
  // compare the kmer first, if they are equal then compare the k
  // colex_varlen_partial_radix_sort_64(uint64_t * a, uint64_t * b, unsigned char * lengths_a, unsigned char * lengths_b, size_t num_records, uint32_t k, uint32_t j, uint64_t ** new_a, uint64_t** new_b, uint64_t ** new_lengths_a, uint64_t ** new_lengths_b);
}

void merge_dummies(FILE * outfile, uint64_t * table_a, uint64_t * table_b, size_t num_records, uint32_t k, uint64_t * incoming_dummies, size_t num_incoming_dummies, unsigned char * dummy_lengths) {
  #define get_a(i) (block_reverse_64(get_left_64(table_a[(i)])))
  #define get_b(i) (block_reverse_64(get_right_64(table_b[(i)], k)))
  #define get_dummy(i) (incoming_dummies[(i)])
  size_t next = 0;
  size_t a_idx = 0, b_idx = 0, d_idx = 0;
  unsigned char d_len = 0, d_len_prev = 0;
  uint64_t x = 0, y = 0, x_prev = 0, y_prev = 0, d = 0, d_prev =0, prev_out = 0;
  char buf[k+1];
  int last=0;

  if (num_records == 0 && num_incoming_dummies == 0) return;

  if (num_records > 0) {
    x = get_a(a_idx);
    y = get_b(b_idx);
    // nothing special about the NOT operation here, just need a guaranteed different value to start with
    x_prev = ~x;
    y_prev = ~y;
  }

  if (num_incoming_dummies > 0) {
    d = get_dummy(d_idx);
    d_len = dummy_lengths[d_idx];
    d_prev = ~d;
    memset(buf, '$', k);
    buf[k] = '\0';
    fprintf(outfile, "%s\n", buf);
  }

  while (a_idx < num_records && b_idx < num_records) {
    // B - A: These y-nodes from table B will require outgoing dummy edges
    while (b_idx < num_records && y < x) {
      // add y to result
      if (y != y_prev) {
        while (d_idx < num_incoming_dummies && (d << 2) <= y) {
          if (d != d_prev && d_len != d_len_prev) {
            // print d
            sprint_dummy_acgt(buf, d, k, d_len);
            fprintf(outfile, "%s\n", buf);
          }
          if (++d_idx >= num_incoming_dummies) break;
          d_prev = d;
          d = incoming_dummies[d_idx];
          d_len_prev = d_len;
          d_len = dummy_lengths[d_idx];
        }
        // Should always print this after the incoming dummies
        uint64_t temp = get_right_64(table_b[b_idx],k) << 2;
        sprint_kmer_acgt(buf, &temp, k);
        buf[k-1] = '$';
        fprintf(outfile, "%s\n", buf);
      }
      if (++b_idx >= num_records) break;
      y_prev = y;
      y = get_b(b_idx);
    }

    // A - B: These x-nodes from table A will require incoming dummy edges
    while (a_idx < num_records && y > x) {
      // add x to result
      if (x != x_prev) {
        // make this already fetched, and x calculated from it
        // if not, continue. otherwise print dummy... (this has to be a loop...)
        while (d_idx < num_incoming_dummies && (d << 2) <= x) {
          if (d != d_prev && d_len != d_len_prev) {
            // print d
            sprint_dummy_acgt(buf, d, k, d_len);
            fprintf(outfile, "%s\n", buf);
          }
          if (++d_idx >= num_incoming_dummies) break;
          d_prev = d;
          d = incoming_dummies[d_idx];
          d_len_prev = d_len;
          d_len = dummy_lengths[d_idx];
        }
        uint64_t temp = table_a[a_idx];
        sprint_kmer_acgt(buf, &temp, k);
        fprintf(outfile, "%s\n", buf);
        //incoming_dummies[next++] = block_reverse_64(temp);
        if (next == num_incoming_dummies) return;
      }
      if (++a_idx >= num_records) break;
      x_prev = x;
      x = get_a(a_idx);
    }

    // These are the nodes that don't need dummy edges
    while (a_idx < num_records && b_idx < num_records && y == x) {
      x_prev = x;
      y_prev = y;

      // Scan to next non-equal (outputing all table entries?)
      while (a_idx < num_records && x == x_prev) {
        while (d_idx < num_incoming_dummies && (d << 2) <= x) {
          if (d != d_prev && d_len != d_len_prev) {
            // print d
            sprint_dummy_acgt(buf, d, k, d_len);
            fprintf(outfile, "%s\n", buf);
          }
          if (++d_idx >= num_incoming_dummies) break;
          d_prev = d;
          d = incoming_dummies[d_idx];
          d_len_prev = d_len;
          d_len = dummy_lengths[d_idx];
        }
        uint64_t temp = table_a[a_idx];
        sprint_kmer_acgt(buf, &temp, k);
        fprintf(outfile, "%s\n", buf);
        if (++a_idx >= num_records) break;
        x_prev = x;
        x = get_a(a_idx);
      }

      // Scan to next non-equal
      while (b_idx < num_records && y == y_prev) {
        if (++b_idx >= num_records) break;
        y_prev = y;
        y = get_b(b_idx);
      }
    }
  }
  last = 1;
  while (d_idx < num_incoming_dummies) {
    if (d != d_prev && d_len != d_len_prev) {
      // print d
      sprint_dummy_acgt(buf, d, k, d_len);
      fprintf(outfile, "%d %s\n", last, buf);
    }
    if (++d_idx >= num_incoming_dummies) break;
    d_prev = d;
    d = incoming_dummies[d_idx];
    d_len_prev = d_len;
    d_len = dummy_lengths[d_idx];
  }
  return;
}
