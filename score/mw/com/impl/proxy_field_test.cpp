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
/// This file contains unit tests for functionality that is unique to fields.
/// There is additional test functionality in the following locations:
///     - score/mw/com/impl/proxy_event_test.cpp contains unit tests which test the event-like functionality of
///     fields.
///     - score/mw/com/impl/bindings/lola/test/proxy_field_component_test.cpp contains component tests which test
///     binding specific field functionality.

#include "score/mw/com/impl/proxy_field.h"

#include "score/mw/com/impl/bindings/mock_binding/proxy.h"
#include "score/mw/com/impl/bindings/mock_binding/proxy_event.h"
#include "score/mw/com/impl/bindings/mock_binding/proxy_method.h"
#include "score/mw/com/impl/com_error.h"
#include "score/mw/com/impl/configuration/lola_service_instance_deployment.h"
#include "score/mw/com/impl/configuration/lola_service_type_deployment.h"
#include "score/mw/com/impl/configuration/quality_type.h"
#include "score/mw/com/impl/configuration/service_identifier_type.h"
#include "score/mw/com/impl/configuration/test/configuration_store.h"
#include "score/mw/com/impl/runtime.h"
#include "score/mw/com/impl/runtime_mock.h"
#include "score/mw/com/impl/test/binding_factory_resources.h"
#include "score/mw/com/impl/test/proxy_resources.h"
#include "score/mw/com/impl/test/runtime_mock_guard.h"

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <type_traits>

namespace score::mw::com::impl
{
namespace
{

using namespace ::testing;

using TestSampleType = std::uint8_t;

class TestProxyBase : public ProxyBase
{
  public:
    using ProxyBase::ProxyBase;
    const ProxyBase::ProxyMethods& GetMethods()
    {
        return methods_;
    }
};

TEST(ProxyFieldTest, NotCopyable)
{
    RecordProperty("Verifies", "SCR-17397027");
    RecordProperty("Description", "Checks copy semantics for ProxyField");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    static_assert(!std::is_copy_constructible<ProxyField<TestSampleType>>::value, "Is wrongly copyable");
    static_assert(!std::is_copy_assignable<ProxyField<TestSampleType>>::value, "Is wrongly copyable");
}

TEST(ProxyFieldTest, IsMoveable)
{
    static_assert(std::is_move_constructible<ProxyField<TestSampleType>>::value, "Is not move constructible");
    static_assert(std::is_move_assignable<ProxyField<TestSampleType>>::value, "Is not move assignable");
}

TEST(ProxyFieldTest, ClassTypeDependsOnFieldDataType)
{
    RecordProperty("Verifies", "SCR-29235459");
    RecordProperty("Description", "ProxyFields with different field data types should be different classes.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    using FirstProxyFieldType = ProxyField<bool>;
    using SecondProxyFieldType = ProxyField<std::uint16_t>;
    static_assert(!std::is_same_v<FirstProxyFieldType, SecondProxyFieldType>,
                  "Class type does not depend on field data type");
}

TEST(ProxyFieldTest, ProxyFieldContainsPublicFieldType)
{
    RecordProperty("Verifies", "SCR-17291997");
    RecordProperty("Description",
                   "A ProxyField contains a public member type FieldType which denotes the type of the field.");
    RecordProperty("TestType", "Requirements-based test");
    RecordProperty("Priority", "1");
    RecordProperty("DerivationTechnique", "Analysis of requirements");

    using CustomFieldType = std::uint16_t;
    static_assert(std::is_same<typename ProxyField<CustomFieldType>::FieldType, CustomFieldType>::value,
                  "Incorrect FieldType.");
}

/// \brief Test fixture for ProxyField with Get and/or Set enabled.
class ProxyFieldGetSetFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        ON_CALL(runtime_mock_guard_.runtime_mock_, GetTracingFilterConfig()).WillByDefault(Return(nullptr));

        ON_CALL(proxy_method_binding_mock_, AllocateReturnType(0))
            .WillByDefault(Return(score::Result<score::cpp::span<std::byte>>{
                score::cpp::span{return_type_buffer_.data(), return_type_buffer_.size()}}));
        ON_CALL(proxy_method_binding_mock_, AllocateInArgs(0))
            .WillByDefault(Return(score::Result<score::cpp::span<std::byte>>{
                score::cpp::span{in_args_buffer_.data(), in_args_buffer_.size()}}));
        ON_CALL(proxy_method_binding_mock_, DoCall(0)).WillByDefault(Return(score::ResultBlank{}));

        ON_CALL(set_method_binding_mock_, AllocateReturnType(0))
            .WillByDefault(Return(score::Result<score::cpp::span<std::byte>>{
                score::cpp::span{return_type_buffer_.data(), return_type_buffer_.size()}}));
        ON_CALL(set_method_binding_mock_, AllocateInArgs(0))
            .WillByDefault(Return(score::Result<score::cpp::span<std::byte>>{
                score::cpp::span{in_args_buffer_.data(), in_args_buffer_.size()}}));
        ON_CALL(set_method_binding_mock_, DoCall(0)).WillByDefault(Return(score::ResultBlank{}));
    }

    alignas(8) std::array<std::byte, 1024> return_type_buffer_{};
    alignas(8) std::array<std::byte, 1024> in_args_buffer_{};
    RuntimeMockGuard runtime_mock_guard_{};
    ConfigurationStore config_store_{InstanceSpecifier::Create(std::string{"/my_dummy_instance_specifier"}).value(),
                                     make_ServiceIdentifierType("foo"),
                                     QualityType::kASIL_QM,
                                     LolaServiceTypeDeployment{42U},
                                     LolaServiceInstanceDeployment{1U}};
    ProxyBase proxy_base_{std::make_unique<mock_binding::Proxy>(), config_store_.GetHandle()};
    mock_binding::ProxyEvent<TestSampleType> proxy_event_mock_{};
    mock_binding::ProxyMethod proxy_method_binding_mock_{};
    mock_binding::ProxyMethod set_method_binding_mock_{};
};

TEST_F(ProxyFieldGetSetFixture, CanConstructProxyFieldWithGetEnabled)
{
    // Given a ProxyField with EnableGet=true
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);
    auto get_binding = std::make_unique<mock_binding::ProxyMethodFacade>(proxy_method_binding_mock_);

    // When constructing the proxy field with Get-only
    ProxyField<TestSampleType, true, false> field{
        proxy_base_, std::move(event_binding), std::move(get_binding), "TestField"};

    // Then it compiles and constructs successfully (no assertion)
}

TEST_F(ProxyFieldGetSetFixture, CanConstructProxyFieldWithSetEnabled)
{
    // Given a ProxyField with EnableSet=true
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);
    auto set_binding = std::make_unique<mock_binding::ProxyMethodFacade>(set_method_binding_mock_);

    // When constructing the proxy field with Set-only
    ProxyField<TestSampleType, false, true> field{
        proxy_base_, std::move(event_binding), std::move(set_binding), "TestField"};

    // Then it compiles and constructs successfully (no assertion)
}

TEST_F(ProxyFieldGetSetFixture, CanConstructProxyFieldWithBothGetAndSetEnabled)
{
    // Given a ProxyField with both EnableGet and EnableSet true
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);
    auto get_binding = std::make_unique<mock_binding::ProxyMethodFacade>(proxy_method_binding_mock_);
    auto set_binding = std::make_unique<mock_binding::ProxyMethodFacade>(set_method_binding_mock_);

    // When constructing the proxy field with both Get and Set
    ProxyField<TestSampleType, true, true> field{
        proxy_base_, std::move(event_binding), std::move(get_binding), std::move(set_binding), "TestField"};

    // Then it compiles and constructs successfully (no assertion)
}

TEST_F(ProxyFieldGetSetFixture, GetDelegatesToProxyMethodBinding)
{
    // Given a ProxyField with EnableGet=true and a mock method binding
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);
    auto get_binding = std::make_unique<mock_binding::ProxyMethodFacade>(proxy_method_binding_mock_);
    ProxyField<TestSampleType, true, false> field{
        proxy_base_, std::move(event_binding), std::move(get_binding), "TestField"};

    // When calling Get()
    EXPECT_CALL(proxy_method_binding_mock_, AllocateReturnType(0));
    EXPECT_CALL(proxy_method_binding_mock_, DoCall(0));
    auto result = field.Get();

    // Then the call is delegated to the underlying method binding
    EXPECT_TRUE(result.has_value());
}

TEST_F(ProxyFieldGetSetFixture, SetDelegatesToProxyMethodBinding)
{
    // Given a ProxyField with EnableSet=true and a mock method binding
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);
    auto set_binding = std::make_unique<mock_binding::ProxyMethodFacade>(set_method_binding_mock_);
    ProxyField<TestSampleType, false, true> field{
        proxy_base_, std::move(event_binding), std::move(set_binding), "TestField"};

    // When calling Set() with a value
    TestSampleType value{42U};
    EXPECT_CALL(set_method_binding_mock_, AllocateInArgs(0));
    EXPECT_CALL(set_method_binding_mock_, AllocateReturnType(0));
    EXPECT_CALL(set_method_binding_mock_, DoCall(0));
    auto result = field.Set(value);

    // Then the call is delegated to the underlying method binding
    EXPECT_TRUE(result.has_value());
}

TEST_F(ProxyFieldGetSetFixture, GetPropagatesBindingError)
{
    // Given a ProxyField with EnableGet=true where the binding returns an error
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);
    auto get_binding = std::make_unique<mock_binding::ProxyMethodFacade>(proxy_method_binding_mock_);
    ProxyField<TestSampleType, true, false> field{
        proxy_base_, std::move(event_binding), std::move(get_binding), "TestField"};

    // When calling Get() and the binding reports an error
    EXPECT_CALL(proxy_method_binding_mock_, AllocateReturnType(0))
        .WillOnce(Return(MakeUnexpected(ComErrc::kBindingFailure)));
    auto result = field.Get();

    // Then the error is propagated back to the caller
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ComErrc::kBindingFailure);
}

TEST_F(ProxyFieldGetSetFixture, SetPropagatesBindingError)
{
    // Given a ProxyField with EnableSet=true where the binding returns an error
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);
    auto set_binding = std::make_unique<mock_binding::ProxyMethodFacade>(set_method_binding_mock_);
    ProxyField<TestSampleType, false, true> field{
        proxy_base_, std::move(event_binding), std::move(set_binding), "TestField"};

    // When calling Set() and the binding reports an error
    TestSampleType value{42U};
    EXPECT_CALL(set_method_binding_mock_, AllocateInArgs(0)).WillOnce(Return(MakeUnexpected(ComErrc::kBindingFailure)));
    auto result = field.Set(value);

    // Then the error is propagated back to the caller
    ASSERT_FALSE(result.has_value());
    EXPECT_EQ(result.error(), ComErrc::kBindingFailure);
}

TEST_F(ProxyFieldGetSetFixture, CanConstructProxyFieldWithNeitherGetNorSet)
{
    // Given a ProxyField with EnableGet=false, EnableSet=false
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);

    // When constructing the proxy field with neither Get nor Set
    ProxyField<TestSampleType, false, false> field{proxy_base_, std::move(event_binding), "TestField"};

    // Then it compiles and constructs successfully (no assertion)
}

/// Tests verifying that ProxyField's Get/Set methods (using FieldOnlyConstructorEnabler) do NOT register
/// as regular methods on the ProxyBase. Only the field itself is registered via RegisterField.

TEST(ProxyFieldFieldOnlyConstructorTest, GetMethodIsNotRegisteredAsRegularMethod)
{
    // Given a TestProxyBase that exposes methods_
    RuntimeMockGuard runtime_mock_guard{};
    ON_CALL(runtime_mock_guard.runtime_mock_, GetTracingFilterConfig()).WillByDefault(Return(nullptr));
    ConfigurationStore config_store{InstanceSpecifier::Create(std::string{"/field_only_ctor_test"}).value(),
                                    make_ServiceIdentifierType("foo"),
                                    QualityType::kASIL_QM,
                                    LolaServiceTypeDeployment{42U},
                                    LolaServiceInstanceDeployment{1U}};
    TestProxyBase proxy_base{std::make_unique<mock_binding::Proxy>(), config_store.GetHandle()};
    mock_binding::ProxyEvent<TestSampleType> event_mock{};
    mock_binding::ProxyMethod method_mock{};

    // When constructing a ProxyField with EnableGet=true (uses FieldOnlyConstructorEnabler internally)
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(event_mock);
    auto get_binding = std::make_unique<mock_binding::ProxyMethodFacade>(method_mock);
    ProxyField<TestSampleType, true, false> field{
        proxy_base, std::move(event_binding), std::move(get_binding), "TestField"};

    // Then no methods are registered on the proxy base (Get method uses FieldOnlyConstructorEnabler)
    EXPECT_TRUE(proxy_base.GetMethods().empty());
}

TEST(ProxyFieldFieldOnlyConstructorTest, SetMethodIsNotRegisteredAsRegularMethod)
{
    // Given a TestProxyBase that exposes methods_
    RuntimeMockGuard runtime_mock_guard{};
    ON_CALL(runtime_mock_guard.runtime_mock_, GetTracingFilterConfig()).WillByDefault(Return(nullptr));
    ConfigurationStore config_store{InstanceSpecifier::Create(std::string{"/field_only_ctor_test"}).value(),
                                    make_ServiceIdentifierType("foo"),
                                    QualityType::kASIL_QM,
                                    LolaServiceTypeDeployment{42U},
                                    LolaServiceInstanceDeployment{1U}};
    TestProxyBase proxy_base{std::make_unique<mock_binding::Proxy>(), config_store.GetHandle()};
    mock_binding::ProxyEvent<TestSampleType> event_mock{};
    mock_binding::ProxyMethod method_mock{};

    // When constructing a ProxyField with EnableSet=true (uses FieldOnlyConstructorEnabler internally)
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(event_mock);
    auto set_binding = std::make_unique<mock_binding::ProxyMethodFacade>(method_mock);
    ProxyField<TestSampleType, false, true> field{
        proxy_base, std::move(event_binding), std::move(set_binding), "TestField"};

    // Then no methods are registered on the proxy base (Set method uses FieldOnlyConstructorEnabler)
    EXPECT_TRUE(proxy_base.GetMethods().empty());
}

TEST(ProxyFieldFieldOnlyConstructorTest, GetAndSetFieldWithNullBindingConstructsWithoutCrash)
{
    // Given a ProxyBase with null method bindings (simulating binding creation failure)
    RuntimeMockGuard runtime_mock_guard{};
    ON_CALL(runtime_mock_guard.runtime_mock_, GetTracingFilterConfig()).WillByDefault(Return(nullptr));
    ConfigurationStore config_store{InstanceSpecifier::Create(std::string{"/field_only_ctor_test"}).value(),
                                    make_ServiceIdentifierType("foo"),
                                    QualityType::kASIL_QM,
                                    LolaServiceTypeDeployment{42U},
                                    LolaServiceInstanceDeployment{1U}};
    TestProxyBase proxy_base{std::make_unique<mock_binding::Proxy>(), config_store.GetHandle()};
    mock_binding::ProxyEvent<TestSampleType> event_mock{};

    // When constructing a ProxyField with both Get and Set but null method bindings
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(event_mock);
    ProxyField<TestSampleType, true, true> field{proxy_base, std::move(event_binding), nullptr, nullptr, "TestField"};

    // Then construction succeeds (FieldOnlyConstructorEnabler marks binding invalid but doesn't crash)
    EXPECT_TRUE(proxy_base.GetMethods().empty());
}

/// Tests for ProxyField production constructors that call the binding factory.

class ProxyFieldProductionCtorFixture : public ::testing::Test
{
  public:
    void SetUp() override
    {
        ON_CALL(runtime_mock_guard_.runtime_mock_, GetTracingFilterConfig()).WillByDefault(Return(nullptr));

        ON_CALL(field_factory_guard_.factory_mock_, CreateEventBinding(_, _))
            .WillByDefault(
                Return(ByMove(std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_))));
        ON_CALL(field_factory_guard_.factory_mock_, CreateGetMethodBinding(_, _))
            .WillByDefault(
                Return(ByMove(std::make_unique<mock_binding::ProxyMethodFacade>(proxy_method_binding_mock_))));
        ON_CALL(field_factory_guard_.factory_mock_, CreateSetMethodBinding(_, _))
            .WillByDefault(Return(ByMove(std::make_unique<mock_binding::ProxyMethodFacade>(set_method_binding_mock_))));
    }

    RuntimeMockGuard runtime_mock_guard_{};
    ConfigurationStore config_store_{InstanceSpecifier::Create(std::string{"/prod_ctor_test"}).value(),
                                     make_ServiceIdentifierType("foo"),
                                     QualityType::kASIL_QM,
                                     LolaServiceTypeDeployment{42U},
                                     LolaServiceInstanceDeployment{1U}};
    ProxyBase proxy_base_{std::make_unique<mock_binding::Proxy>(), config_store_.GetHandle()};
    mock_binding::ProxyEvent<TestSampleType> proxy_event_mock_{};
    mock_binding::ProxyMethod proxy_method_binding_mock_{};
    mock_binding::ProxyMethod set_method_binding_mock_{};
    ProxyFieldBindingFactoryMockGuard<TestSampleType> field_factory_guard_{};
};

TEST_F(ProxyFieldProductionCtorFixture, ProductionCtorWithGetAndSetCallsAllBindingFactories)
{
    // Given a proxy base and binding factory mock that returns valid bindings
    // Then all three factory methods are called
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateEventBinding(_, _));
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateGetMethodBinding(_, _));
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateSetMethodBinding(_, _));

    // When constructing a ProxyField<T, true, true> via the production constructor
    ProxyField<TestSampleType, true, true> field{proxy_base_, "TestField"};
}

TEST_F(ProxyFieldProductionCtorFixture, ProductionCtorWithGetOnlyCallsEventAndGetFactories)
{
    // Given a proxy base and binding factory mock
    // Then event and get factory methods are called, but NOT set
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateEventBinding(_, _));
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateGetMethodBinding(_, _));
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateSetMethodBinding(_, _)).Times(0);

    // When constructing a ProxyField<T, true, false> via the production constructor
    ProxyField<TestSampleType, true, false> field{proxy_base_, "TestField"};
}

TEST_F(ProxyFieldProductionCtorFixture, ProductionCtorWithSetOnlyCallsEventAndSetFactories)
{
    // Given a proxy base and binding factory mock
    // Then event and set factory methods are called, but NOT get
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateEventBinding(_, _));
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateGetMethodBinding(_, _)).Times(0);
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateSetMethodBinding(_, _));

    // When constructing a ProxyField<T, false, true> via the production constructor
    ProxyField<TestSampleType, false, true> field{proxy_base_, "TestField"};
}

TEST_F(ProxyFieldProductionCtorFixture, ProductionCtorWithNeitherCallsOnlyEventFactory)
{
    // Given a proxy base and binding factory mock
    // Then only event factory method is called
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateEventBinding(_, _));
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateGetMethodBinding(_, _)).Times(0);
    EXPECT_CALL(field_factory_guard_.factory_mock_, CreateSetMethodBinding(_, _)).Times(0);

    // When constructing a ProxyField<T, false, false> via the production constructor
    ProxyField<TestSampleType, false, false> field{proxy_base_, "TestField"};
}

/// Tests for ProxyField move semantics with Get/Set dispatchers.

TEST_F(ProxyFieldGetSetFixture, MoveConstructedProxyFieldWithGetPreservesGetFunctionality)
{
    // Given a ProxyField with EnableGet=true
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);
    auto get_binding = std::make_unique<mock_binding::ProxyMethodFacade>(proxy_method_binding_mock_);
    ProxyField<TestSampleType, true, false> original{
        proxy_base_, std::move(event_binding), std::move(get_binding), "TestField"};

    // When move-constructing a new ProxyField from the original
    ProxyField<TestSampleType, true, false> moved{std::move(original)};

    // Then Get() still works on the moved-to field
    EXPECT_CALL(proxy_method_binding_mock_, AllocateReturnType(0));
    EXPECT_CALL(proxy_method_binding_mock_, DoCall(0));
    auto result = moved.Get();
    EXPECT_TRUE(result.has_value());
}

TEST_F(ProxyFieldGetSetFixture, MoveConstructedProxyFieldWithSetPreservesSetFunctionality)
{
    // Given a ProxyField with EnableSet=true
    auto event_binding = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);
    auto set_binding = std::make_unique<mock_binding::ProxyMethodFacade>(set_method_binding_mock_);
    ProxyField<TestSampleType, false, true> original{
        proxy_base_, std::move(event_binding), std::move(set_binding), "TestField"};

    // When move-constructing a new ProxyField from the original
    ProxyField<TestSampleType, false, true> moved{std::move(original)};

    // Then Set() still works on the moved-to field
    TestSampleType value{42U};
    EXPECT_CALL(set_method_binding_mock_, AllocateInArgs(0));
    EXPECT_CALL(set_method_binding_mock_, AllocateReturnType(0));
    EXPECT_CALL(set_method_binding_mock_, DoCall(0));
    auto result = moved.Set(value);
    EXPECT_TRUE(result.has_value());
}

TEST_F(ProxyFieldGetSetFixture, MoveAssignedProxyFieldWithGetPreservesGetFunctionality)
{
    // Given two ProxyFields with EnableGet=true
    auto event_binding1 = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(proxy_event_mock_);
    auto get_binding1 = std::make_unique<mock_binding::ProxyMethodFacade>(proxy_method_binding_mock_);
    ProxyField<TestSampleType, true, false> original{
        proxy_base_, std::move(event_binding1), std::move(get_binding1), "TestField"};

    mock_binding::ProxyEvent<TestSampleType> other_event_mock{};
    mock_binding::ProxyMethod other_method_mock{};
    auto event_binding2 = std::make_unique<mock_binding::ProxyEventFacade<TestSampleType>>(other_event_mock);
    auto get_binding2 = std::make_unique<mock_binding::ProxyMethodFacade>(other_method_mock);
    ProxyField<TestSampleType, true, false> target{
        proxy_base_, std::move(event_binding2), std::move(get_binding2), "OtherField"};

    // When move-assigning the original to the target
    target = std::move(original);

    // Then Get() works on the moved-to field using original's binding
    EXPECT_CALL(proxy_method_binding_mock_, AllocateReturnType(0));
    EXPECT_CALL(proxy_method_binding_mock_, DoCall(0));
    auto result = target.Get();
    EXPECT_TRUE(result.has_value());
}

}  // namespace
}  // namespace score::mw::com::impl
