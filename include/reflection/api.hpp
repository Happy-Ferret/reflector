/*
    Boost Software License - Version 1.0 - August 17, 2003

    Permission is hereby granted, free of charge, to any person or organization
    obtaining a copy of the software and accompanying documentation covered by
    this license (the "Software") to use, reproduce, display, distribute,
    execute, and transmit the Software, and to prepare derivative works of the
    Software, and to permit third-parties to whom the Software is furnished to
    do so, all subject to the following:

    The copyright notices in the Software and this entire statement, including
    the above license grant, this restriction and the following disclaimer,
    must be included in all copies of the Software, in whole or in part, and
    all derivative works of the Software, unless such copies or derivative
    works are solely in the form of machine-executable object code generated by
    a source language processor.

    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
    SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
    FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
    ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
    DEALINGS IN THE SOFTWARE.
*/

#pragma once

#include "bufstring.hpp"
#include "base.hpp"

#ifndef REFLECTOR_AVOID_STL
#include <string>
#endif

namespace reflection {  // UUID('c3549467-1615-4087-9829-176a2dc44b76')

extern IErrorHandler* err;

class ReflectedFields {
public:
    struct Field : Field_t {
        const char* className;
        void* inst;
        void* field;

        Field(const Field_t& field_in, const char* className, void* inst)
            : Field_t(field_in), className(className), inst(inst) {
            field = fieldGetter(inst);
        }

        operator void* () { return field; }
        operator const void* () const { return field; }

        bool deserialize(IReader* reader) { return refl->deserialize(err, reader, field); }
        bool isPolymorphic() const { return refl->isPolymorphic(); }
        template <typename T> bool isType() const { return refl == reflectionForType2<T>(); }
        const char* staticTypeName() const { return refl->staticTypeName(); }
        bool toString(char*& buf, size_t& bufSize) const { return refl->toString(err, buf, bufSize, FIELD_STATE, field); }
        bool toString(IErrorHandler* err, char*& buf, size_t& bufSize) const { return refl->toString(err, buf, bufSize, FIELD_STATE, field); }
        const char* typeName() const { return refl->typeName(field); }
        bool serialize(IWriter* writer) const { return refl->serialize(err, writer, field); }
        bool setFromString(IErrorHandler* err, const char* str) { return refl->setFromString(err, str, strlen(str), field); }

#ifndef REFLECTOR_AVOID_STL
        bool setFromString(const std::string& str) { return refl->setFromString(err, str.c_str(), str.length(), field); }

        std::string toString() const {
            char* buf = nullptr;
            size_t bufSize = 0;
            AllocGuard guard(buf);

            if (!refl->toString(err, buf, bufSize, FIELD_STATE, field))
                return "";

            return buf;
        }
#endif
    };

    ReflectedFields(void* inst, FieldSet_t const* fieldSet)
            : inst(inst), fieldSet(fieldSet) {
        // count all fields including base class(es)
        numFields = 0;
        for (FieldSet_t const* p_fieldSet = fieldSet; p_fieldSet != nullptr; p_fieldSet = p_fieldSet->baseClassFields) {
            numFields += p_fieldSet->numFields;
        }
    }

    Field operator [] (size_t index) const {
        FieldSet_t const* p_fieldSet = fieldSet;
        void* p_inst = inst;

        while (true) {
            // perhaps the field is in this class?
            if (index < p_fieldSet->numFields)
                return Field(p_fieldSet->fields[index], p_fieldSet->className, p_inst);

            // apparently not; search next base class
            index -= p_fieldSet->numFields;
            p_inst = p_fieldSet->derivedPtrToBasePtr(p_inst);
            p_fieldSet = p_fieldSet->baseClassFields;
        }
    }

    size_t count() const {
        return numFields;
    }

    operator size_t () const {  // deprecated
        return numFields;
    }

    void* inst;
    FieldSet_t const* fieldSet;
    size_t numFields;
};

template <typename T>
const char* reflectClassName(T& inst) {
    return inst.reflection_className(REFL_MATCH);
}

template <typename T>
ReflectedFields reflectFields(T& inst) {
    return ReflectedFields((void*) &inst, inst.reflection_getFields(REFL_MATCH));
}

template <typename C>
ReflectedFields reflectFieldsStatic() {
    return ReflectedFields(nullptr, C::template reflection_s_getFields<C>(REFL_MATCH));
}

template <typename T>
void reflectPrint(T& instance, uint32_t fieldMask = FIELD_STATE | FIELD_CONFIG) {
    const char* className = reflectClassName(instance);
    printf("Instance of class %s:\n", className);

    auto fields = reflectFields(instance);

    for (size_t i = 0; i < fields; i++) {
        const auto& field = fields[i];

        if (!(field.systemFlags & fieldMask))
            continue;

        printf("%-15s %s::%s = %s\n", field.typeName(), field.className, field.name, field.toString().c_str());

        if (field.isPolymorphic())
            printf("\t(declared field type: %s)\n", field.staticTypeName());
    }

    printf("\n");
}

template <typename T>
bool reflectSerialize(T& inst, IWriter* writer) {
    ITypeReflection* refl = reflectionForType(inst);

    return refl->serialize(err, writer, (const void*) &inst);
}

template <typename T>
bool reflectDeserialize(T& value_out, IReader* reader) {
    ITypeReflection* refl = reflectionForType(value_out);

    return refl->deserialize(err, reader, (void*) &value_out);
}

#ifndef REFLECTOR_AVOID_STL
template <typename T>
std::string reflectToString(T& inst, uint32_t fieldMask = FIELD_STATE) {
    ITypeReflection* refl = reflectionForType(inst);

    char* buf = nullptr;
    size_t bufSize = 0;
    AllocGuard guard(buf);

    if (!refl->toString(err, buf, bufSize, fieldMask, (const void*) &inst))
        return "";

    return buf;
}
#endif

template <typename T>
bool reflectFromString(T& inst, const std::string& str) {
    ITypeReflection* refl = reflectionForType(inst);

    return refl->setFromString(err, str.c_str(), str.length(), (void*) &inst);
}

template <typename T>
const char* reflectTypeName() {
    auto refl = reflectionForType2<T>();

    return refl->staticTypeName();
}

template <typename T>
const char* reflectTypeName(T& inst) {
    auto refl = reflectionForType2<T>();

    return refl->typeName((const void*) &inst);
}

template <class C>
const UUID_t& uuidOfClass() {
    return C::reflection_s_uuid(REFL_MATCH);
}

template <class C>
const char* versionedNameOfClass() {
    return C::reflection_s_classId(REFL_MATCH);
}
}
