/* Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The ASF licenses this file to You under the Apache License, Version 2.0
 * (the "License"); you may not use this file except in compliance with
 * the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#define C_CFISH_BYTEBUF
#define CFISH_USE_SHORT_NAMES

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#include "Clownfish/Class.h"
#include "Clownfish/ByteBuf.h"
#include "Clownfish/Blob.h"
#include "Clownfish/Err.h"
#include "Clownfish/String.h"
#include "Clownfish/Util/Memory.h"

static void
S_grow(ByteBuf *self, size_t size);

ByteBuf*
BB_new(size_t capacity) {
    ByteBuf *self = (ByteBuf*)Class_Make_Obj(BYTEBUF);
    return BB_init(self, capacity);
}

ByteBuf*
BB_init(ByteBuf *self, size_t capacity) {
    size_t amount = capacity ? capacity : sizeof(int64_t);
    self->buf   = NULL;
    self->size  = 0;
    self->cap   = 0;
    S_grow(self, amount);
    return self;
}

ByteBuf*
BB_new_bytes(const void *bytes, size_t size) {
    ByteBuf *self = (ByteBuf*)Class_Make_Obj(BYTEBUF);
    return BB_init_bytes(self, bytes, size);
}

ByteBuf*
BB_init_bytes(ByteBuf *self, const void *bytes, size_t size) {
    BB_init(self, size);
    memcpy(self->buf, bytes, size);
    self->size = size;
    return self;
}

ByteBuf*
BB_new_steal_bytes(void *bytes, size_t size, size_t capacity) {
    ByteBuf *self = (ByteBuf*)Class_Make_Obj(BYTEBUF);
    return BB_init_steal_bytes(self, bytes, size, capacity);
}

ByteBuf*
BB_init_steal_bytes(ByteBuf *self, void *bytes, size_t size,
                    size_t capacity) {
    self->buf  = (char*)bytes;
    self->size = size;
    self->cap  = capacity;
    return self;
}

void
BB_Destroy_IMP(ByteBuf *self) {
    FREEMEM(self->buf);
    SUPER_DESTROY(self, BYTEBUF);
}

ByteBuf*
BB_Clone_IMP(ByteBuf *self) {
    return BB_new_bytes(self->buf, self->size);
}

void
BB_Set_Size_IMP(ByteBuf *self, size_t size) {
    if (size > self->cap) {
        THROW(ERR, "Can't set size to %u64 (greater than capacity of %u64)",
              (uint64_t)size, (uint64_t)self->cap);
    }
    self->size = size;
}

char*
BB_Get_Buf_IMP(ByteBuf *self) {
    return self->buf;
}

size_t
BB_Get_Size_IMP(ByteBuf *self) {
    return self->size;
}

size_t
BB_Get_Capacity_IMP(ByteBuf *self) {
    return self->cap;
}

static CFISH_INLINE bool
SI_equals_bytes(ByteBuf *self, const void *bytes, size_t size) {
    if (self->size != size) { return false; }
    return (memcmp(self->buf, bytes, self->size) == 0);
}

bool
BB_Equals_IMP(ByteBuf *self, Obj *other) {
    ByteBuf *const twin = (ByteBuf*)other;
    if (twin == self)              { return true; }
    if (!Obj_is_a(other, BYTEBUF)) { return false; }
    return SI_equals_bytes(self, twin->buf, twin->size);
}

bool
BB_Equals_Bytes_IMP(ByteBuf *self, const void *bytes, size_t size) {
    return SI_equals_bytes(self, bytes, size);
}

static CFISH_INLINE void
SI_mimic_bytes(ByteBuf *self, const void *bytes, size_t size) {
    if (size > self->cap) { S_grow(self, size); }
    memmove(self->buf, bytes, size);
    self->size = size;
}

void
BB_Mimic_Bytes_IMP(ByteBuf *self, const void *bytes, size_t size) {
    SI_mimic_bytes(self, bytes, size);
}

void
BB_Mimic_IMP(ByteBuf *self, Obj *other) {
    if (Obj_is_a(other, BYTEBUF)) {
        ByteBuf *twin = (ByteBuf*)other;
        SI_mimic_bytes(self, twin->buf, twin->size);
    }
    else if (Obj_is_a(other, STRING)) {
        String *string = (String*)other;
        SI_mimic_bytes(self, Str_Get_Ptr8(string), Str_Get_Size(string));
    }
    else {
        THROW(ERR, "ByteBuf can't mimic %o", Obj_get_class_name(other));
    }
}

static CFISH_INLINE void
SI_cat_bytes(ByteBuf *self, const void *bytes, size_t size) {
    const size_t new_size = self->size + size;
    if (new_size > self->cap) {
        S_grow(self, Memory_oversize(new_size, sizeof(char)));
    }
    memcpy((self->buf + self->size), bytes, size);
    self->size = new_size;
}

void
BB_Cat_Bytes_IMP(ByteBuf *self, const void *bytes, size_t size) {
    SI_cat_bytes(self, bytes, size);
}

void
BB_Cat_IMP(ByteBuf *self, Blob *blob) {
    SI_cat_bytes(self, Blob_Get_Buf(blob), Blob_Get_Size(blob));
}

static void
S_grow(ByteBuf *self, size_t size) {
    if (size > self->cap) {
        size_t amount    = size;
        size_t remainder = amount % sizeof(int64_t);
        if (remainder) {
            amount += sizeof(int64_t);
            amount -= remainder;
        }
        self->buf = (char*)REALLOCATE(self->buf, amount);
        self->cap = amount;
    }
}

char*
BB_Grow_IMP(ByteBuf *self, size_t size) {
    if (size > self->cap) { S_grow(self, size); }
    return self->buf;
}

Blob*
BB_Yield_Blob_IMP(ByteBuf *self) {
    Blob *blob = Blob_new_steal(self->buf, self->size);
    self->buf  = NULL;
    self->size = 0;
    self->cap  = 0;
    return blob;
}

int
BB_compare(const void *va, const void *vb) {
    ByteBuf *a = *(ByteBuf**)va;
    ByteBuf *b = *(ByteBuf**)vb;
    const size_t size = a->size < b->size ? a->size : b->size;

    int32_t comparison = memcmp(a->buf, b->buf, size);

    if (comparison == 0 && a->size != b->size) {
        comparison = a->size < b->size ? -1 : 1;
    }

    return comparison;
}

int32_t
BB_Compare_To_IMP(ByteBuf *self, Obj *other) {
    CERTIFY(other, BYTEBUF);
    return BB_compare(&self, &other);
}


