#include <iostream>
#include <list>
#include <ctgmath>
#include <cstdlib>
#include <bitset>
#include <iomanip>
#include <limits.h>

#define BIT_POS(first, order) (((first) >> (order)) + 1)
#define BITMAP_XOR(first, order) bitmaps[order][BIT_POS(first, order)] = 1

/* TODO: custom bitset */
#define MAX_BIT_SIZE (1024>>1)+1

using namespace std;

typedef struct {
    int first;
    int last;
} ListElmt;

class BuddyAllocator {
    private:
        int base;
        int total_size;
        int max_order;
        list<ListElmt> *free_area;
        bitset<MAX_BIT_SIZE> *bitmaps;

    public:
        BuddyAllocator(int first, int last, int max_order);
        int GetOrder(int size);
        int Allocate(int size);
        void Deallocate(int first, int size);
        void Merge(int order, int first);
        void Dump(void);
};

int BuddyAllocator::GetOrder(int size)
{
    return ceil(log2(size));
}

BuddyAllocator::BuddyAllocator(int first, int last, int max_order=10)
{
    base = first;
    total_size = last - first + 1;
    int order = int(log2(total_size));
    max_order = min(order, max_order);
   
    bitmaps = new bitset<MAX_BIT_SIZE>[max_order+1];
    free_area = new list<ListElmt>[max_order+1];

    int block_size, block_num, block_first = 0, total = total_size;
    ListElmt elmt;
    for (order = max_order; order >= 0 && total > 0; order--) {
        block_size = int(pow(2, order));
        block_num = total / block_size;
        while (block_num-- > 0) {
            elmt.first = block_first;
            elmt.last = block_first + block_size - 1;
            free_area[order].push_front(elmt);
            BITMAP_XOR(elmt.first, order);
            block_first += block_size;
        }
        total = total % block_size;
    }
}

int BuddyAllocator::Allocate(int size)
{
    if (size <= 0 || size > total_size)
        return INT_MAX;

    int order = GetOrder(size);
    ListElmt elmt;
    while (true) {
        if (!free_area[order].empty()) {
            elmt = free_area[order].front();
            free_area[order].pop_front();
            BITMAP_XOR(elmt.first, order);
            return base + elmt.first;
        }
        else if (order < max_order) {
            order++;
            if (!free_area[order].empty()) {
                elmt = free_area[order].front();
                free_area[order].pop_front();
                BITMAP_XOR(elmt.first, order);

                int mid = (elmt.first + elmt.last)/2;
                ListElmt prev_elmt = { elmt.first, mid };
                ListElmt next_elmt = { mid+1, elmt.last };
                free_area[order-1].push_front(prev_elmt);
                free_area[order-1].push_front(next_elmt);
                order = GetOrder(size);
            }
        }
        else {
            break;
        }
    }
    return INT_MAX;
}

void BuddyAllocator::Deallocate(int first, int size)
{
    int order = GetOrder(size);
    Merge(order, first-base);
}

void BuddyAllocator::Merge(int order, int first)
{
    list<ListElmt> &area = free_area[order];
    list<ListElmt>::iterator it;
    int hit, merge_first = INT_MAX;

    ListElmt elmt = { first, first+int(pow(2, order))-1 };
    area.push_front(elmt);
    BITMAP_XOR(elmt.first, order);

    if (bitmaps[order].test(BIT_POS(first, order)) || order == max_order)
        return;

    for (hit = 0, it = area.begin(); it != area.end() && hit < 2; it++) {
        if (BIT_POS(it->first, order) == BIT_POS(first, order)) {
            hit += 1;
            merge_first = min(it->first, merge_first);
            area.erase(it);
        }
    }

    Merge(order+1, merge_first);
}

void BuddyAllocator::Dump(void)
{
    for (int order = 0; order <= max_order; order++) {
        list<ListElmt> area = free_area[order];
        cout << "|ORDER " << setw(2) << setfill('0') << order << "|";
        if (!area.empty()) {
            list<ListElmt>::iterator it;
            for (it = area.begin(); it != area.end(); it++) {
                cout << " ---> [" << setw(4) << setfill(' ') << it->first << ", ";
                cout << setw(4) << setfill(' ') << it->last << "] ";
            }
            cout << endl;
        }
        else
            cout << endl;
    }
    cout << endl;
}
#define TEST_MIN 1
#define TEST_MAX 1024

typedef struct {
    int id;
    int sz;
} test_id;

int main(int argc, char *argv[])
{
    int count = argc > 1 ? atoi(argv[1]) : 1;

    BuddyAllocator buddy(TEST_MIN, TEST_MAX);

    list<test_id> ids;
    srand(time(nullptr));

    while (count--) {
        bool action = rand()%2;
        if (action) {
            int sz = (rand()%8)+1;
            int id = buddy.Allocate(sz);
            if (id == INT_MAX)
                break;
            cout << "Allocated size " << sz << " and first id is " << id << ", ";
            buddy.Dump();
            test_id node = { id, sz };
            ids.push_front(node);
        }
        else {
            if (ids.empty())
                continue;
            
            cout << "Deallocated id " << ids.back().id << " and size is " << ids.back().sz << endl;
            buddy.Deallocate(ids.back().id, ids.back().sz);
            ids.pop_back();
            buddy.Dump();
        }
    }
    return 0;
}
