#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t low = (uint32_t)challenge;
    uint32_t high = (uint32_t)(challenge >> 32);
    uint32_t middle = (uint32_t)(challenge >> 16);
    uint32_t first = (high ^ UINT32_C(0x9eee)) + 2U * low;
    uint32_t second = (middle + UINT32_C(0x94fb)) ^ high;
    uint32_t third = (3U * second + low) ^ UINT32_C(0xed48);
    values[0] = third ^ first;
    return write_answer(out, cap, values, count);
}

int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 6;
    values[0] = (((uint32_t)(challenge >> 32) * 2U) & 30U) + 31U;
    for (uint32_t i = 1; i < count; ++i) {
        unsigned shift = 3U * i + 4U;
        uint32_t part = (uint32_t)((challenge >> shift) & 63U);
        values[i] = 5U * values[i - 1] + (part ^ (i + 10U));
    }
    return write_answer(out, cap, values, count);
}

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    uint32_t selector = (uint32_t)challenge & 7U;
    uint32_t x = (uint32_t)(challenge >> 3) ^ (uint32_t)(challenge >> 32);
    values[0] = selector;
    /* The jump-table order differs from the p3_case_N symbol order. */
    switch (selector) {
        case 0: values[1] = (4U * x) ^ UINT32_C(0x268e); break;
        case 1: values[1] = 5U * x + UINT32_C(0x23bc2); break;
        case 2: values[1] = (2U * x) ^ UINT32_C(0xa07f); break;
        case 3: values[1] = 5U * x + UINT32_C(0x2ec5c); break;
        case 4: values[1] = (x ^ UINT32_C(0x289d)) + (x >> 2); break;
        case 5: values[1] = (x ^ UINT32_C(0xf7a8)) + (x >> 1); break;
        case 6: values[1] = 5U * x + UINT32_C(0x53d9); break;
        case 7: values[1] = 5U * x + UINT32_C(0xd620); break;
    }
    return write_answer(out, cap, values, count);
}

static uint32_t walk_path(uint32_t target, uint32_t low, uint32_t high, uint32_t salt) {
    uint32_t middle = low + ((high - low) >> 1);
    if (target == middle) return (middle + 100U) ^ salt;
    uint32_t child;
    uint32_t branch;
    if (target < middle) {
        child = walk_path(target, low, middle - 1U, salt);
        branch = 8U;
    } else {
        child = walk_path(target, middle + 1U, high, salt);
        branch = 10U;
    }
    return (3U * child) ^ (middle + salt) ^ branch;
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    uint32_t target = (uint32_t)challenge & 127U;
    uint32_t salt = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    values[0] = walk_path(target, 0U, 127U, salt);
    return write_answer(out, cap, values, count);
}

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    /* Binary layout: next pointer at +0, identifier at +8, value at +12.
       Store links as indices here so the solution does not depend on addresses. */
    static const struct {
        unsigned next;
        uint32_t identifier;
        uint32_t value;
    } nodes[] = {
        {9,  UINT32_C(0x81f), UINT32_C(0xd329)},
        {4,  UINT32_C(0x364), UINT32_C(0x741b)},
        {3,  UINT32_C(0x25f), UINT32_C(0x8c1d)},
        {7,  UINT32_C(0x926), UINT32_C(0xe495)},
        {8,  UINT32_C(0x751), UINT32_C(0xe5fd)},
        {12, UINT32_C(0x44c), UINT32_C(0x3468)},
        {11, UINT32_C(0x634), UINT32_C(0xae19)},
        {1,  UINT32_C(0xa0d), UINT32_C(0xa615)},
        {6,  UINT32_C(0x136), UINT32_C(0x9b98)},
        {5,  UINT32_C(0x028), UINT32_C(0x75d6)},
        {2,  UINT32_C(0x52d), UINT32_C(0x192d)},
        {0,  UINT32_C(0xb1b), UINT32_C(0x7a93)}
    };
    static const unsigned heads[] = {10, 2, 3, 7, 1, 4, 8, 6};
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 2;
    unsigned node = heads[challenge & 7U];
    unsigned visits = (unsigned)((challenge >> 3) & 3U) + 1U;
    uint32_t accumulator = (uint32_t)(challenge >> 6) ^
                           (uint32_t)(challenge >> 32) ^ UINT32_C(0xb9ee);
    for (unsigned i = 0; i < visits; ++i) {
        if (node >= sizeof(nodes) / sizeof(nodes[0])) return 1;
        accumulator = (3U * accumulator + nodes[node].value) ^ nodes[node].identifier;
        values[1] = nodes[node].identifier;
        node = nodes[node].next;
    }
    values[0] = accumulator;
    return write_answer(out, cap, values, count);
}

static uint32_t apply_operation(unsigned index, uint32_t x) {
    static const unsigned operations[] = {3, 2, 0, 3, 1, 4, 5, 5};
    switch (operations[index]) {
        case 0: return (x + UINT32_C(0x8056)) ^ (x >> 5);
        case 1: return (x ^ UINT32_C(0x4239)) + (x << 5);
        case 2: return ((x << 1) | (x >> 31)) ^ UINT32_C(0x4954);
        case 3: return (2U * x + UINT32_C(0x6d1d)) ^ (3U * x);
        case 4: return (x ^ (x >> 7)) + UINT32_C(0x62c0);
        case 5: return (x >> 7) | (x << 25);
    }
    return 0;
}

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    size_t count = 1;
    unsigned first = (unsigned)(challenge & 7U);
    unsigned second = (unsigned)((challenge >> 3) & 7U);
    uint32_t x = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    x = apply_operation(first, x) ^ UINT32_C(0x1464);
    values[0] = apply_operation(second, x) + UINT32_C(0x519e);
    return write_answer(out, cap, values, count);
}

int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t index = (uint32_t)context & 3U;
    uint32_t challenge_fold = (uint32_t)challenge ^ (uint32_t)(challenge >> 32);
    uint32_t context_fold = (uint32_t)context ^ (uint32_t)(context >> 32);
    uint32_t x = challenge_fold ^ context_fold;
    uint32_t mixed = ((x >> 5) ^ UINT32_C(0x345f)) + (x ^ UINT32_C(0xf220));
    uint32_t second = mixed ^ (index * UINT32_C(0xf220)) ^ (uint32_t)(context >> 16);
    uint32_t third = second ^ ((challenge_fold >> 5) + context_fold +
                               index + UINT32_C(0x345f));
    /* Offset 32 holds a little-endian 0x4e45504f ("OPEN"); the numeric
       answer starts at offset 36. Only adjacent data fields are overwritten. */
    int size = snprintf(out, cap,
                        "AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAOPEN%" PRIu32 " %" PRIu32 " %" PRIu32,
                        index, second, third);
    return size < 0 || (size_t)size >= cap;
}
