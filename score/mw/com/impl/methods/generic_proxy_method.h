/********************************************************************************
 * Copyright (c) 2026 Contributors to the Eclipse Foundation
 *
 * See the NOTICE file(s) distributed with this work for additional
 * information regarding copyright ownership.
 *
 * This program and the accompanying materials are made available under the
 * terms of the Apache License Version 2.0 which is available at
 * https://www.apache.org/licenses/LICENSE-2.0
 *
 * SPDX-License-Identifier: Apache-2.0
 ********************************************************************************/
#ifndef SCORE_MW_COM_IMPL_METHODS_GENERIC_PROXY_METHOD_H
#define SCORE_MW_COM_IMPL_METHODS_GENERIC_PROXY_METHOD_H

#include "score/mw/com/impl/methods/proxy_method_base.h"
#include "score/mw/com/impl/methods/proxy_method_binding.h"
#include "score/mw/com/impl/proxy_base.h"

#include "score/result/result.h"

#include <score/span.hpp>

#include <cstddef>
#include <memory>
#include <string_view>

namespace score::mw::com::impl
{

/// \brief User-visible type-erased method class that is part of a generic proxy.
/// \details Like ProxyMethod<Signature> but without compile-time knowledge of the method's argument
///          and return types. The in-args and return value are exchanged as raw byte buffers so a
///          generic proxy (e.g. a recorder or a routing layer) can invoke a method purely from
///          runtime metadata.
///
///          The class delegates all work to an underlying ProxyMethodBinding, which is already
///          type-erased (operates on span<std::byte> + queue_position). Binding-specific details
///          (allocating call-queue slots, issuing the actual call) live in the binding
///          implementation, e.g. lola::ProxyMethod.
class GenericProxyMethod final : public ProxyMethodBase
{
  public:
    /// \brief Construct a GenericProxyMethod by attaching a ready-made binding.
    /// \details Used both in tests (with a mock binding) and in production (with a binding
    ///          produced by the generic factory path). If binding is nullptr the parent proxy is
    ///          marked as having an invalid service-element binding, matching the behaviour of
    ///          the typed ProxyMethod constructors.
    GenericProxyMethod(ProxyBase& parent,
                       std::unique_ptr<ProxyMethodBinding> binding,
                       std::string_view method_name) noexcept;

    /// Production ctor. Obtains the binding from GenericProxyMethodBindingFactory, which reads
    /// the method's size info from shared memory and builds the appropriate lola::ProxyMethod.
    GenericProxyMethod(ProxyBase& parent, std::string_view method_name) noexcept;

    ~GenericProxyMethod() final = default;

    GenericProxyMethod(const GenericProxyMethod&) = delete;
    GenericProxyMethod& operator=(const GenericProxyMethod&) = delete;

    GenericProxyMethod(GenericProxyMethod&&) noexcept;
    GenericProxyMethod& operator=(GenericProxyMethod&&) noexcept;

    /// \brief Ask the binding for a byte buffer to write the call's in-arguments into.
    /// \note Must not be called if the method has no in-arguments.
    score::Result<score::cpp::span<std::byte>> AllocateInArgs(std::size_t queue_position);

    /// \brief Ask the binding for a byte buffer where the call's return value will be written.
    /// \note Must not be called if the method has a void return type.
    score::Result<score::cpp::span<std::byte>> AllocateReturnType(std::size_t queue_position);

    /// \brief Invoke the method at the given call-queue position. In-args (if any) must already
    ///        be written by the caller and the return buffer (if any) must already be allocated.
    score::ResultBlank Call(std::size_t queue_position);
};

}  // namespace score::mw::com::impl

#endif  // SCORE_MW_COM_IMPL_METHODS_GENERIC_PROXY_METHOD_H
