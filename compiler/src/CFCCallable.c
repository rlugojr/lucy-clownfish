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

#include <string.h>
#include <ctype.h>

#ifndef true
    #define true 1
    #define false 0
#endif

#define CFC_NEED_CALLABLE_STRUCT_DEF
#include "CFCCallable.h"
#include "CFCClass.h"
#include "CFCParcel.h"
#include "CFCType.h"
#include "CFCParamList.h"
#include "CFCVariable.h"
#include "CFCDocuComment.h"
#include "CFCUtil.h"

static const CFCMeta CFCCALLABLE_META = {
    "Clownfish::CFC::Model::Callable",
    sizeof(CFCCallable),
    (CFCBase_destroy_t)CFCCallable_destroy
};

CFCCallable*
CFCCallable_init(CFCCallable *self, CFCParcel *parcel, const char *exposure,
                 const char *class_name, const char *class_nickname,
                 const char *micro_sym, CFCType *return_type,
                 CFCParamList *param_list, CFCDocuComment *docucomment) {

    exposure = exposure ? exposure : "parcel";
    CFCUTIL_NULL_CHECK(class_name);
    CFCUTIL_NULL_CHECK(return_type);
    CFCUTIL_NULL_CHECK(param_list);
    CFCSymbol_init((CFCSymbol*)self, parcel, exposure, class_name,
                   class_nickname, micro_sym);
    self->return_type = (CFCType*)CFCBase_incref((CFCBase*)return_type);
    self->param_list  = (CFCParamList*)CFCBase_incref((CFCBase*)param_list);
    self->docucomment = (CFCDocuComment*)CFCBase_incref((CFCBase*)docucomment);
    return self;
}

void
CFCCallable_destroy(CFCCallable *self) {
    CFCBase_decref((CFCBase*)self->return_type);
    CFCBase_decref((CFCBase*)self->param_list);
    CFCBase_decref((CFCBase*)self->docucomment);
    CFCSymbol_destroy((CFCSymbol*)self);
}

void
CFCCallable_resolve_types(CFCCallable *self) {
    CFCType_resolve(self->return_type);
    CFCParamList_resolve_types(self->param_list);
}

int
CFCCallable_can_be_bound(CFCCallable *self) {
    // Test whether parameters can be mapped automatically.
    CFCVariable **arg_vars = CFCParamList_get_variables(self->param_list);
    for (size_t i = 0; arg_vars[i] != NULL; i++) {
        CFCType *type = CFCVariable_get_type(arg_vars[i]);
        if (!CFCType_is_object(type) && !CFCType_is_primitive(type)) {
            return false;
        }
    }

    // Test whether return type can be mapped automatically.
    if (!CFCType_is_void(self->return_type)
        && !CFCType_is_object(self->return_type)
        && !CFCType_is_primitive(self->return_type)
    ) {
        return false;
    }

    return true;
}

