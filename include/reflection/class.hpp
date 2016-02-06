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

#include "api.hpp"
#include "serialization_manager.hpp"

namespace reflection {  // UUID('c3549467-1615-4087-9829-176a2dc44b76')

template <typename Fields>
bool fieldsToBufString(IErrorHandler* err, char*& buf, size_t& bufSize, const Fields& fields, uint32_t fieldMask) {
    if (!bufStringSet(err, buf, bufSize, "{", 1))
        return false;

    char* fieldAsString = nullptr;
    size_t fieldAsStringSize = 0;

    AllocGuard guard(fieldAsString);
    bool first = true;

    for (size_t i = 0; i < fields.count(); i++) {
        const auto& field = fields[i];

        if (!(field.systemFlags & fieldMask))
            continue;

        if (!first) {
            if (!bufStringAppend(err, buf, bufSize, ", ", 2))
                return false;
        }
        else
            first = false;

        if (!field.toString(err, fieldAsString, fieldAsStringSize))
            return false;

        if (!bufStringAppend(err, buf, bufSize, field.name, strlen(field.name))
                || !bufStringAppend(err, buf, bufSize, "=\"", 2)
                || !bufStringAppend(err, buf, bufSize, fieldAsString, strlen(fieldAsString))
                || !bufStringAppend(err, buf, bufSize, "\"", 1))
            return false;

    }

    if (!bufStringAppend(err, buf, bufSize, "}", 1))
        return false;

    return true;
}

template <class C>
class ClassReflection : public ITypeReflection {
    virtual bool isPolymorphic() override {
        return false;
    }

    virtual const UUID_t* uuidOrNull(const void* p_value) override {
        const C& instance = *reinterpret_cast<const C*>(p_value);

        return instance.reflection_uuidOrNull(REFL_MATCH);
    }

    virtual const char* staticTypeName() override {
        return C::reflection_s_className(REFL_MATCH);
    }

    virtual const char* typeName(const void* p_value) override {
        const C& instance = *reinterpret_cast<const C*>(p_value);
        return instance.reflection_className(REFL_MATCH);
    }

    virtual bool serialize(IErrorHandler* err, serialization::IWriter* writer, const void* p_value) override {
        const C& instance = *reinterpret_cast<const C*>(p_value);

        const auto fields = reflectFields(instance);

        return serialization::SerializationManager<C>::serializeInstance(
                err, writer, instance.reflection_classId(REFL_MATCH), fields);
    }

    virtual bool deserialize(IErrorHandler* err, serialization::IReader* reader, void* p_value) override {
        C& instance = *reinterpret_cast<C*>(p_value);

        auto fields = reflectFields(instance);

        return serialization::SerializationManager<C>::deserializeInstance(
                err, reader, instance.reflection_classId(REFL_MATCH), fields);
    }

    virtual bool serializeTypeInformation(IErrorHandler* err, serialization::IWriter* writer, const void* p_value) override {
        if (p_value != nullptr) {
            const C& instance = *reinterpret_cast<const C*>(p_value);

            return serialization::SerializationManager<C>::serializeInstanceTypeInformation(err, writer, instance);
        }
        else
            return serialization::SerializationManager<C>::serializeInstanceTypeInformation(err, writer);
    }

    virtual bool verifyTypeInformation(IErrorHandler* err, serialization::IReader* reader, void* p_value) override {
        C& instance = *reinterpret_cast<C*>(p_value);

        return serialization::SerializationManager<C>::verifyInstanceTypeInformation(err, reader, instance);
    }

    virtual bool setFromString(IErrorHandler* err, const char* str, size_t strLen,
            void* p_value) override {
        return err->notImplemented("reflection::ClassReflection::setFromString"), false;
    }

    virtual bool toString(IErrorHandler* err, char*& buffer, size_t& bufferSize, uint32_t fieldMask,
            const void* p_value) override {
        const C& instance = *reinterpret_cast<const C*>(p_value);

        const auto fields = reflectFields(instance);
        return fieldsToBufString(err, buffer, bufferSize, fields, fieldMask);
    }
};

template <class C>
ITypeReflection* reflectionForType(C) {
    static ClassReflection<C> reflection;
    return &reflection;
}

template <class C>
struct ReflectionForType2
{
    static ITypeReflection* reflectionForType2() {
        static ClassReflection<C> reflection;
        return &reflection;
    }
};
}
