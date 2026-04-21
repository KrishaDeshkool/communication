/********************************************************************************
 * Copyright (c) 2025 Contributors to the Eclipse Foundation
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
// Unit tests for ProxyFieldBindingFactory event binding creation are implemented in
// proxy_service_element_binding_factory_test.cpp (proxy_event_field_binding_factory_test.cpp).
// Tests specific to CreateGetMethodBinding and CreateSetMethodBinding are below.

#include "score/mw/com/impl/plumbing/proxy_field_binding_factory.h"

#include "score/mw/com/impl/bindings/lola/element_fq_id.h"
#include "score/mw/com/impl/bindings/lola/proxy.h"
#include "score/mw/com/impl/bindings/lola/test/proxy_event_test_resources.h"
#include "score/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "score/mw/com/impl/configuration/lola_service_instance_id.h"
#include "score/mw/com/impl/configuration/quality_type.h"
#include "score/mw/com/impl/configuration/service_identifier_type.h"
#include "score/mw/com/impl/configuration/test/configuration_store.h"
#include "score/mw/com/impl/handle_type.h"
#include "score/mw/com/impl/instance_identifier.h"
#include "score/mw/com/impl/proxy_base.h"
#include "score/mw/com/impl/test/dummy_instance_identifier_builder.h"

#include <gtest/gtest.h>
#include <cstdint>
#include <memory>
#include <utility>

namespace score::mw::com::impl
{
namespace
{

using namespace ::testing;

using TestSampleType = std::uint8_t;

constexpr auto kDummyFieldName{"Field1"};
constexpr std::uint16_t kDummyFieldId{6U};
constexpr uint16_t kInstanceId = 0x31U;
const LolaServiceId kServiceId{1U};
constexpr lola::SkeletonEventProperties kSkeletonEventProperties{5U, 3U, true};
const auto kInstanceSpecifier = InstanceSpecifier::Create(std::string{"/my_dummy_instance_specifier"}).value();

const LolaServiceInstanceDeployment kLolaServiceInstanceDeployment{
    LolaServiceInstanceId{kInstanceId},
    {},
    {{kDummyFieldName, LolaFieldInstanceDeployment{{1U}, {3U}, 1U, true, 0}}}};
const LolaServiceTypeDeployment kLolaServiceTypeDeployment{kServiceId, {}, {{kDummyFieldName, kDummyFieldId}}};

constexpr auto kQualityType = QualityType::kASIL_B;
ConfigurationStore kConfigStore{kInstanceSpecifier,
                                make_ServiceIdentifierType("/a/service/somewhere/out/there", 13U, 37U),
                                kQualityType,
                                kLolaServiceTypeDeployment,
                                kLolaServiceInstanceDeployment};

class ProxyFieldBindingFactoryFixture : public lola::ProxyMockedMemoryFixture
{
  public:
    ProxyFieldBindingFactoryFixture& WithAProxyBaseWithValidBinding(const HandleType& handle)
    {
        proxy_base_ = std::make_unique<ProxyBase>(std::move(proxy_), handle);
        return *this;
    }

    ProxyFieldBindingFactoryFixture& WithAProxyBaseWithNullBinding(const HandleType& handle)
    {
        proxy_base_ = std::make_unique<ProxyBase>(nullptr, handle);
        return *this;
    }

    std::unique_ptr<ProxyBase> proxy_base_{nullptr};
    DummyInstanceIdentifierBuilder dummy_instance_identifier_builder_{};
};

TEST_F(ProxyFieldBindingFactoryFixture, CreateGetMethodBindingReturnsValidBinding)
{
    // Given a valid proxy with a lola binding and a field deployment
    const auto instance_identifier = kConfigStore.GetInstanceIdentifier();
    const auto handle = kConfigStore.GetHandle();
    const auto element_fq_id = lola::ElementFqId{kServiceId, kDummyFieldId, kInstanceId, ServiceElementType::FIELD};
    InitialiseDummySkeletonEvent(element_fq_id, kSkeletonEventProperties);
    InitialiseProxyWithConstructor(instance_identifier);
    WithAProxyBaseWithValidBinding(handle);

    // When creating a Get method binding
    auto binding = ProxyFieldBindingFactory<TestSampleType>::CreateGetMethodBinding(*proxy_base_, kDummyFieldName);

    // Then a valid binding is returned
    EXPECT_NE(binding, nullptr);
}

TEST_F(ProxyFieldBindingFactoryFixture, CreateSetMethodBindingReturnsValidBinding)
{
    // Given a valid proxy with a lola binding and a field deployment
    const auto instance_identifier = kConfigStore.GetInstanceIdentifier();
    const auto handle = kConfigStore.GetHandle();
    const auto element_fq_id = lola::ElementFqId{kServiceId, kDummyFieldId, kInstanceId, ServiceElementType::FIELD};
    InitialiseDummySkeletonEvent(element_fq_id, kSkeletonEventProperties);
    InitialiseProxyWithConstructor(instance_identifier);
    WithAProxyBaseWithValidBinding(handle);

    // When creating a Set method binding
    auto binding = ProxyFieldBindingFactory<TestSampleType>::CreateSetMethodBinding(*proxy_base_, kDummyFieldName);

    // Then a valid binding is returned
    EXPECT_NE(binding, nullptr);
}

TEST_F(ProxyFieldBindingFactoryFixture, CreateGetMethodBindingReturnsNullptrForNonLolaBinding)
{
    // Given a proxy base with no lola binding
    const auto handle = kConfigStore.GetHandle();
    WithAProxyBaseWithNullBinding(handle);

    // When creating a Get method binding
    auto binding = ProxyFieldBindingFactory<TestSampleType>::CreateGetMethodBinding(*proxy_base_, kDummyFieldName);

    // Then nullptr is returned
    EXPECT_EQ(binding, nullptr);
}

TEST_F(ProxyFieldBindingFactoryFixture, CreateSetMethodBindingReturnsNullptrForNonLolaBinding)
{
    // Given a proxy base with no lola binding
    const auto handle = kConfigStore.GetHandle();
    WithAProxyBaseWithNullBinding(handle);

    // When creating a Set method binding
    auto binding = ProxyFieldBindingFactory<TestSampleType>::CreateSetMethodBinding(*proxy_base_, kDummyFieldName);

    // Then nullptr is returned
    EXPECT_EQ(binding, nullptr);
}

}  // namespace
}  // namespace score::mw::com::impl
