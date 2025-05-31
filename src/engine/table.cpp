#include "table.h"

namespace transposition
{

u16 Entry::get_hash()
{
    return this->hash;
};

u16 Entry::get_move()
{
    return this->move;
};

i32 Entry::get_score(i32 ply)
{
    if (this->score == eval::score::NONE) {
        return this->score;
    }

    // Mate scores need to be adjusted
    // Otherwise, we will not know if we have a faster or slower mate
    if (this->score >= eval::score::MATE_FOUND) {
        return this->score - ply;
    }
    else if (this->score <= -eval::score::MATE_FOUND) {
        return this->score + ply;
    }

    return this->score;
};

i32 Entry::get_eval()
{
    return this->eval;
};

i32 Entry::get_depth()
{
    return this->depth;
};

u8 Entry::get_age()
{
    return this->flags >> 3;
};

u8 Entry::get_bound()
{
    return this->flags & mask::BOUND;
};

i32 Entry::get_age_distance(u8 age)
{
    return (MAX_AGE + age - this->get_age()) % MAX_AGE;
};

bool Entry::is_pv()
{
    return this->flags & mask::PV;
};

void Entry::set_score(i32 score, i32 ply)
{
    // Mate scores need to be adjusted
    if (score != eval::score::NONE) {
        if (score >= eval::score::MATE_FOUND) {
            score += ply;
        }
        else if (score <= -eval::score::MATE_FOUND) {
            score -= ply;
        }
    }

    this->score = score;
};

void Entry::set(u64 hash, u16 move, i32 score, i32 eval, i32 depth, u8 age, bool pv, u8 bound, i32 ply)
{
    // Preserves any existing move for the same position
    if (move || this->hash != static_cast<u16>(hash)) {
        this->move = move;
    }

    // Overwrites less valuable entry
    if (bound == bound::EXACT ||
        this->hash != static_cast<u16>(hash) ||
        this->get_age() != age ||
        depth + 4 + 2 * i32(pv) > this->depth) {
        this->hash = static_cast<u16>(hash);
        this->depth = static_cast<u8>(depth);
        this->set_score(score, ply);
        this->eval = eval;
        this->flags = (age << 3) | (u8(pv) << 2) | bound;
    }
};

Table::Table()
{
    this->buckets = nullptr;
    this->count = 0;
    this->age = 0;
};

Table::~Table()
{
    if (this->buckets != nullptr) {
        free_aligned(this->buckets);
    }
};

void Table::init(u64 mb)
{
    // Free memory if there is any
    if (this->buckets != nullptr) {
        free_aligned(this->buckets);
    }

    // Sets count
    this->count = mb * MB / sizeof(Bucket);
    
    // Alloc memory
    #if defined(__linux__)
    u64 alignment = 2 * MB;
    #else
    u64 alignment = 4 * KB;
    #endif

    this->buckets = static_cast<Bucket*>(malloc_aligned(alignment, mb * MB));

    this->age = 0;
};

void Table::clear(usize thread_count)
{
    if (this->buckets == nullptr) {
        return;
    }

    const u64 chunk = this->count / thread_count;

    std::vector<std::thread> threads;

    for (usize i = 0; i < thread_count; ++i) {
        threads.emplace_back([&] (usize thread_id) {
            std::memset((void*)&this->buckets[chunk * thread_id], 0, chunk * sizeof(Bucket));
        }, i);
    }

    if (chunk * thread_count < this->count) {
        std::memset((void*)&this->buckets[chunk * thread_count], 0, (this->count - chunk * thread_count) * sizeof(Bucket));
    }

    for (auto& t : threads) {
        t.join();
    }

    this->age = 0;
};

std::pair<bool, Entry*> Table::get(u64 hash)
{
    const u64 index = this->get_index(hash);

    auto entries = this->buckets[index].entries;

    // Finds matching entry
    for (usize i = 0; i < MAX_ENTRIES; ++i) {
        if (entries[i].get_hash() == static_cast<u16>(hash)) {
            return { true, &entries[i] };
        }
    }

    // Finds replacement
    auto entry = &entries[0];

    for (usize i = 1; i < MAX_ENTRIES; ++i) {
        if (entries[i].get_depth() - 4 * entries[i].get_age_distance(this->age) <
            entry->get_depth() - 4 * entry->get_age_distance(this->age)) {
            entry = &entries[i];
        }
    }

    return { false, entry };
};

u64 Table::get_index(u64 hash)
{
    return static_cast<u64>((static_cast<u128>(hash) * static_cast<u128>(this->count)) >> 64);
};

void Table::update()
{
    this->age = (this->age + 1) % MAX_AGE;
};

void Table::prefetch(u64 hash)
{
    __builtin_prefetch(&this->buckets[this->get_index(hash)]);
};

usize Table::hashfull()
{
    usize count = 0;

    for (usize i = 0; i < 1000; ++i) {
        for (usize k = 0; k < MAX_ENTRIES; ++k) {
            if (this->buckets[i].entries[k].get_age() == this->age &&
                this->buckets[i].entries[k].get_hash() != 0) {
                count += 1;
            }
        }
    }

    return count / MAX_ENTRIES;
};


};