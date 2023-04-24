#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define DIMENSIONS 2  // Number of dimensions for each point
#define MAX_ENTRIES 4  // Maximum number of entries in a node

typedef struct {
    double point[DIMENSIONS];
} Point;

typedef struct {
    Point min_corner;
    Point max_corner;
} Rectangle;

typedef struct HRTreeNode {
    Rectangle rect;
    int num_entries;
    void *entries[MAX_ENTRIES];
    struct HRTreeNode *children[MAX_ENTRIES];
    int is_leaf;
} HRTreeNode;

// Create a new point with the specified coordinates
Point new_point(double x, double y) {
    Point point;
    point.point[0] = x;
    point.point[1] = y;
    return point;
}

// Create a new rectangle with the specified corners
Rectangle new_rectangle(Point min_corner, Point max_corner) {
    Rectangle rect;
    rect.min_corner = min_corner;
    rect.max_corner = max_corner;
    return rect;
}

// Calculate the minimum bounding rectangle that contains two rectangles
Rectangle mbr(Rectangle rect1, Rectangle rect2) {
    Point min_corner, max_corner;
    int i;
    for (i = 0; i < DIMENSIONS; i++) {
        min_corner.point[i] = fmin(rect1.min_corner.point[i], rect2.min_corner.point[i]);
        max_corner.point[i] = fmax(rect1.max_corner.point[i], rect2.max_corner.point[i]);
    }
    return new_rectangle(min_corner, max_corner);
}

// Calculate the area of a rectangle
double area(Rectangle rect) {
    double a = 1;
    int i;
    for (i = 0; i < DIMENSIONS; i++) {
        a *= rect.max_corner.point[i] - rect.min_corner.point[i];
    }
    return a;
}

// Check if two rectangles overlap
int overlap(Rectangle rect1, Rectangle rect2) {
    int i;
    for (i = 0; i < DIMENSIONS; i++) {
        if (rect1.max_corner.point[i] < rect2.min_corner.point[i] ||
            rect1.min_corner.point[i] > rect2.max_corner.point[i]) {
            return 0;
        }
    }
    return 1;
}

// Choose two entries to be the seeds for a new node
void choose_seeds(HRTreeNode *node, int *seed1, int *seed2) {
    int i, j;
    double max_distance = -1;
    for (i = 0; i < node->num_entries; i++) {
        for (j = i + 1; j < node->num_entries; j++) {
            double distance = 0;
            int k;
            for (k = 0; k < DIMENSIONS; k++) {
                distance += pow(node->entries[i]->point[k] - node->entries[j]->point[k], 2);
            }
            if (distance > max_distance) {
                max_distance = distance;
                *seed1 = i;
                *seed2 = j;
            }
        }
    }
}

// Add an entry to a node
void add_entry(HRTreeNode *node, void *entry) {
    node->entries[node->num_entries] = entry;
    node->rect = mbr(node->rect, ((Point *)entry)->point, DIMENSIONS);
    node->num_entries++;
}

// Add a child node to a node
void add_child(HRTreeNode *node, HRTreeNode *child) {
    node->children[node->num_entries] = child;
    node->rect = mbr(node->rect, child->rect);
    node->num_entries++;
}
// Split a node into two new nodes using the quadratic split algorithm
void split_node(HRTreeNode *node, HRTreeNode **new_node1, HRTreeNode **new_node2) {
    int i, j, seed1, seed2;
    choose_seeds(node, &seed1, &seed2);
    *new_node1 = malloc(sizeof(HRTreeNode));
    *new_node2 = malloc(sizeof(HRTreeNode));
    (*new_node1)->is_leaf = node->is_leaf;
    (*new_node2)->is_leaf = node->is_leaf;
    add_entry(*new_node1, node->entries[seed1]);
    add_entry(*new_node2, node->entries[seed2]);
    for (i = 0; i < node->num_entries; i++) {
        if (i != seed1 && i != seed2) {
            HRTreeNode *target_node = area((*new_node1)->rect) < area((*new_node2)->rect) ? *new_node1 : *new_node2;
            double enlargement1 = area(mbr(target_node->rect, node->entries[i]->point, DIMENSIONS)) - area(target_node->rect);
            double enlargement2 = area(mbr((*new_node1)->rect, (*new_node2)->rect)) - area((*new_node1)->rect) - area((*new_node2)->rect);
            j = enlargement1 < enlargement2 ? 0 : 1;
            add_entry(target_node->children[j], node->entries[i]);
            add_child(target_node, target_node->children[j]);
        }
    }
}

// Insert an entry into the Hilbert R-tree
void insert_entry(HRTreeNode **root, void *entry) {
    if (*root == NULL) {
        *root = malloc(sizeof(HRTreeNode));
        (*root)->is_leaf = 1;
        add_entry(*root, entry);
        return;
    }
    if ((*root)->is_leaf && (*root)->num_entries == MAX_ENTRIES) {
        HRTreeNode *new_node1, *new_node2;
        split_node(*root, &new_node1, &new_node2);
        *root = malloc(sizeof(HRTreeNode));
        (*root)->is_leaf = 0;
        add_child(*root, new_node1);
        add_child(*root, new_node2);
    }
    if (!(*root)->is_leaf) {
        int i, best_child_index = -1;
        double best_enlargement = -1;
        for (i = 0; i < (*root)->num_entries; i++) {
            double enlargement = area(mbr((*root)->children[i]->rect, ((Point *)entry)->point, DIMENSIONS)) - area((*root)->children[i]->rect);
            if (enlargement < best_enlargement || best_child_index == -1) {
                best_child_index = i;
                best_enlargement = enlargement;
            } else if (enlargement == best_enlargement && area((*root)->children[i]->rect) < area((*root)->children[best_child_index]->rect)) {
                best_child_index = i;
            }
        }
        insert_entry(&((*root)->children[best_child_index]), entry);
        (*root)->rect = mbr((*root)->rect, (*root)->children[best_child_index]->rect);
    }
    if ((*root)->is_leaf && (*root)->num_entries < MAX_ENTRIES) {
        add_entry(*root, entry);
    }
}

// Search for all entries in the tree that overlap the given rectangle
void search(HRTreeNode *node, Rect rect, void (*callback)(void *)) {
    int i;
    if (intersects(node->rect, rect)) {
        if (node->is_leaf) {
            for (i = 0; i < node->num_entries; i++) {
                if (intersects(node->entries[i]->rect, rect)) {
                    print_intersecting_rectangles(node->entries[i]->data);
                }
            }
        } else {
            for (i = 0; i < node->num_entries; i++) {
                if (intersects(node->children[i]->rect, rect)) {
                    search(node->children[i], rect, callback);
                }
            }
        }
    }
}

void print_intersecting_rectangles(void *data) {
    HRTreeData *d = (HRTreeData *)data;
    printf("Intersecting rectangle: (%d, %d) - (%d, %d)\n", 
           d->rect.xmin, d->rect.ymin, d->rect.xmax, d->rect.ymax);
}


// Helper function to calculate the Hilbert value of a point
int hilbert_value(int x, int y) {
    int hilbert_order = 32; // Set Hilbert curve order to 32
    int hilbert_size = (1 << hilbert_order);
    int hilbert_index = 0;
    for (int s = hilbert_size / 2; s > 0; s /= 2) {
        int rx = (x & s) > 0;
        int ry = (y & s) > 0;
        hilbert_index += s * s * ((3 * rx) ^ ry);
        rotate(s, &x, &y, rx, ry);
    }
    return hilbert_index;
}

// Helper function to perform rotation of coordinates
void rotate(int n, int *x, int *y, int rx, int ry) {
    if (ry == 0) {
        if (rx == 1) {
            *x = n - 1 - *x;
            *y = n - 1 - *y;
        }
        int temp = *x;
        *x = *y;
        *y = temp;
    }
}

// Helper function to calculate the minimum bounding rectangle of two rectangles
Rect mbr(Rect r1, Rect r2) {
    Rect r = {0};
    r.xmin = (r1.xmin < r2.xmin) ? r1.xmin : r2.xmin;
    r.ymin = (r1.ymin < r2.ymin) ? r1.ymin : r2.ymin;
    r.xmax = (r1.xmax > r2.xmax) ? r1.xmax : r2.xmax;
    r.ymax = (r1.ymax > r2.ymax) ? r1.ymax : r2.ymax;
    return r;
}

// Helper function to check whether two rectangles intersect
int intersects(Rect r1, Rect r2) {
    return !(r2.xmin > r1.xmax || r2.xmax < r1.xmin || r2.ymin > r1.ymax || r2.ymax < r1.ymin);
}

void preorder_traversal(HRTreeNode *node, void (*callback)(void *)) {
    if (node != NULL) {
        print_hilbert_value(node->data);
        if (!node->is_leaf) {
            for (int i = 0; i < node->num_entries; i++) {
                preorder_traversal(node->children[i], callback);
            }
        }
    }
}

void print_hilbert_value(void *data) {
    HRTreeData *d = (HRTreeData *)data;
    printf("Hilbert value: %d\n", d->hilbert_value);
}

// node represents a tree node of the tree.
// C code based on explanation of ChatGPT
// struct HRtree {
//     int min, max, bits;
//     struct node *root;
//     struct h *hf;
//     int size;
// };
// struct HRtree *new_tree(int min, int max, int bits) {
//     struct HRtree *rt = malloc(sizeof(struct HRtree));
//     if (rt == NULL) {
//         perror("malloc error");
//         exit(EXIT_FAILURE);
//     }

//     rt->hf = NULL;
//     rt->root = NULL;

//     if (bits < DEFAULT_RESOLUTION) {
//         bits = DEFAULT_RESOLUTION;
//     }

//     if (min < 0) {
//         min = DEFAULT_MIN_NODE_ENTRIES;
//     }

//     if (max < 0) {
//         max = DEFAULT_MAX_NODE_ENTRIES;
//     }

//     if (max < min) {
//         fprintf(stderr, "Minimum number of nodes should be less than Maximum number of nodes and not vice versa.\n");
//         exit(EXIT_FAILURE);
//     }

//     struct h *hf = NULL;
//     int err = h_new(&hf, (uint32_t) bits, DIM);

//     if (err != 0) {
//         fprintf(stderr, "Error creating Hilbert curve instance\n");
//         exit(EXIT_FAILURE);
//     }

//     struct node *root = new_node(min, max);
//     root->leaf = 1;

//     rt->min = min;
//     rt->max = max;
//     rt->bits = bits;
//     rt->hf = hf;
//     rt->root = root;

//     return rt;
// }
// type node struct
// {
//     min, max int
//     parent *node
//     left, right *node
//     leaf bool
//     entries *entryList
//      lhv *big.Int
//      bb *rectangle // bounding-box of all children of this entry
// }
// type entry struct {
// 	bb   *rectangle // bounding-box of of this entry
// 	node *node
// 	obj  Rectangle
// 	h    *big.Int // hilbert value
// 	leaf bool
// }
