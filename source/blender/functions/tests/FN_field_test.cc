/* Apache License, Version 2.0 */

#include "testing/testing.h"

#include "FN_cpp_type.hh"
#include "FN_field.hh"
#include "FN_multi_function_builder.hh"

namespace blender::fn::tests {

TEST(field, ConstantFunction)
{
  /* TODO: Figure out how to not use another "OperationFieldSource(" inside of std::make_shared. */
  GField constant_field{std::make_shared<OperationFieldSource>(OperationFieldSource(
                            std::make_unique<CustomMF_Constant<int>>(10), {})),
                        0};

  Array<int> result(4);
  GMutableSpan result_generic(result.as_mutable_span());
  FieldContext field_context;
  evaluate_fields_to_spans(
      {&constant_field}, IndexMask(IndexRange(4)), field_context, {result_generic});

  EXPECT_EQ(result[0], 10);
  EXPECT_EQ(result[1], 10);
  EXPECT_EQ(result[2], 10);
  EXPECT_EQ(result[3], 10);
}

class IndexContextFieldSource final : public ContextFieldSource {
 public:
  IndexContextFieldSource() : ContextFieldSource(CPPType::get<int>(), "Index")
  {
  }

  const GVArray *try_get_varray_for_context(const FieldContext &UNUSED(context),
                                            IndexMask mask,
                                            ResourceScope &scope) const final
  {
    auto index_func = [](int i) { return i; };
    return &scope.construct<
        GVArray_For_EmbeddedVArray<int, VArray_For_Func<int, decltype(index_func)>>>(
        __func__, mask.min_array_size(), mask.min_array_size(), index_func);
  }
};

TEST(field, VArrayInput)
{
  GField index_field{std::make_shared<IndexContextFieldSource>()};

  Array<int> result_1(4);
  GMutableSpan result_generic_1(result_1.as_mutable_span());
  FieldContext field_context;
  evaluate_fields_to_spans(
      {&index_field}, IndexMask(IndexRange(4)), field_context, {result_generic_1});
  EXPECT_EQ(result_1[0], 0);
  EXPECT_EQ(result_1[1], 1);
  EXPECT_EQ(result_1[2], 2);
  EXPECT_EQ(result_1[3], 3);

  /* Evaluate a second time, just to test that the first didn't break anything. */
  Array<int> result_2(10);
  GMutableSpan result_generic_2(result_2.as_mutable_span());
  evaluate_fields_to_spans({&index_field}, {2, 4, 6, 8}, field_context, {result_generic_2});
  EXPECT_EQ(result_2[2], 2);
  EXPECT_EQ(result_2[4], 4);
  EXPECT_EQ(result_2[6], 6);
  EXPECT_EQ(result_2[8], 8);
}

TEST(field, VArrayInputMultipleOutputs)
{
  std::shared_ptr<ContextFieldSource> index_input = std::make_shared<IndexContextFieldSource>();
  GField field_1{index_input};
  GField field_2{index_input};

  Array<int> result_1(10);
  Array<int> result_2(10);
  GMutableSpan result_generic_1(result_1.as_mutable_span());
  GMutableSpan result_generic_2(result_2.as_mutable_span());

  FieldContext field_context;
  evaluate_fields_to_spans(
      {&field_1, &field_2}, {2, 4, 6, 8}, field_context, {result_generic_1, result_generic_2});
  EXPECT_EQ(result_1[2], 2);
  EXPECT_EQ(result_1[4], 4);
  EXPECT_EQ(result_1[6], 6);
  EXPECT_EQ(result_1[8], 8);
  EXPECT_EQ(result_2[2], 2);
  EXPECT_EQ(result_2[4], 4);
  EXPECT_EQ(result_2[6], 6);
  EXPECT_EQ(result_2[8], 8);
}

TEST(field, InputAndFunction)
{
  GField index_field{std::make_shared<IndexContextFieldSource>()};

  std::unique_ptr<MultiFunction> add_fn = std::make_unique<CustomMF_SI_SI_SO<int, int, int>>(
      "add", [](int a, int b) { return a + b; });
  GField output_field{std::make_shared<OperationFieldSource>(
                          OperationFieldSource(std::move(add_fn), {index_field, index_field})),
                      0};

  Array<int> result(10);
  GMutableSpan result_generic(result.as_mutable_span());
  FieldContext field_context;
  evaluate_fields_to_spans({&output_field}, {2, 4, 6, 8}, field_context, {result_generic});
  EXPECT_EQ(result[2], 4);
  EXPECT_EQ(result[4], 8);
  EXPECT_EQ(result[6], 12);
  EXPECT_EQ(result[8], 16);
}

TEST(field, TwoFunctions)
{
  GField index_field{std::make_shared<IndexContextFieldSource>()};

  std::unique_ptr<MultiFunction> add_fn = std::make_unique<CustomMF_SI_SI_SO<int, int, int>>(
      "add", [](int a, int b) { return a + b; });
  GField add_field{std::make_shared<OperationFieldSource>(
                       OperationFieldSource(std::move(add_fn), {index_field, index_field})),
                   0};

  std::unique_ptr<MultiFunction> add_10_fn = std::make_unique<CustomMF_SI_SO<int, int>>(
      "add_10", [](int a) { return a + 10; });
  GField result_field{std::make_shared<OperationFieldSource>(
                          OperationFieldSource(std::move(add_10_fn), {add_field})),
                      0};

  Array<int> result(10);
  GMutableSpan result_generic(result.as_mutable_span());
  FieldContext field_context;
  evaluate_fields_to_spans({&result_field}, {2, 4, 6, 8}, field_context, {result_generic});
  EXPECT_EQ(result[2], 14);
  EXPECT_EQ(result[4], 18);
  EXPECT_EQ(result[6], 22);
  EXPECT_EQ(result[8], 26);
}

class TwoOutputFunction : public MultiFunction {
 private:
  MFSignature signature_;

 public:
  TwoOutputFunction(StringRef name)
  {
    MFSignatureBuilder signature{name};
    signature.single_input<int>("In1");
    signature.single_input<int>("In2");
    signature.single_output<int>("Add");
    signature.single_output<int>("Add10");
    signature_ = signature.build();
    this->set_signature(&signature_);
  }

  void call(IndexMask mask, MFParams params, MFContext UNUSED(context)) const override
  {
    const VArray<int> &in1 = params.readonly_single_input<int>(0, "In1");
    const VArray<int> &in2 = params.readonly_single_input<int>(1, "In2");
    MutableSpan<int> add = params.uninitialized_single_output<int>(2, "Add");
    MutableSpan<int> add_10 = params.uninitialized_single_output<int>(3, "Add10");
    mask.foreach_index([&](const int64_t i) {
      add[i] = in1[i] + in2[i];
      add_10[i] = add[i] + 10;
    });
  }
};

TEST(field, FunctionTwoOutputs)
{
  /* Also use two separate input fields, why not. */
  GField index_field_1{std::make_shared<IndexContextFieldSource>()};
  GField index_field_2{std::make_shared<IndexContextFieldSource>()};

  std::shared_ptr<OperationFieldSource> fn = std::make_shared<OperationFieldSource>(
      OperationFieldSource(std::make_unique<TwoOutputFunction>("SI_SI_SO_SO"),
                           {index_field_1, index_field_2}));

  GField result_field_1{fn, 0};
  GField result_field_2{fn, 1};

  Array<int> result_1(10);
  Array<int> result_2(10);
  GMutableSpan result_generic_1(result_1.as_mutable_span());
  GMutableSpan result_generic_2(result_2.as_mutable_span());
  FieldContext field_context;
  evaluate_fields_to_spans({&result_field_1, &result_field_2},
                           {2, 4, 6, 8},
                           field_context,
                           {result_generic_1, result_generic_2});
  EXPECT_EQ(result_1[2], 4);
  EXPECT_EQ(result_1[4], 8);
  EXPECT_EQ(result_1[6], 12);
  EXPECT_EQ(result_1[8], 16);
  EXPECT_EQ(result_2[2], 14);
  EXPECT_EQ(result_2[4], 18);
  EXPECT_EQ(result_2[6], 22);
  EXPECT_EQ(result_2[8], 26);
}

TEST(field, TwoFunctionsTwoOutputs)
{
  GField index_field{std::make_shared<IndexContextFieldSource>()};

  std::shared_ptr<OperationFieldSource> fn = std::make_shared<OperationFieldSource>(
      OperationFieldSource(std::make_unique<TwoOutputFunction>("SI_SI_SO_SO"),
                           {index_field, index_field}));

  Vector<int64_t> mask_indices = {2, 4, 6, 8};
  IndexMask mask = mask_indices.as_span();

  Field<int> result_field_1{fn, 0};
  Field<int> intermediate_field{fn, 1};

  std::unique_ptr<MultiFunction> add_10_fn = std::make_unique<CustomMF_SI_SO<int, int>>(
      "add_10", [](int a) { return a + 10; });
  Field<int> result_field_2{std::make_shared<OperationFieldSource>(
                                OperationFieldSource(std::move(add_10_fn), {intermediate_field})),
                            0};

  FieldContext field_context;
  FieldEvaluator field_evaluator{field_context, &mask};
  const VArray<int> *result_1 = nullptr;
  const VArray<int> *result_2 = nullptr;
  field_evaluator.add(result_field_1, &result_1);
  field_evaluator.add(result_field_2, &result_2);
  field_evaluator.evaluate();

  EXPECT_EQ(result_1->get(2), 4);
  EXPECT_EQ(result_1->get(4), 8);
  EXPECT_EQ(result_1->get(6), 12);
  EXPECT_EQ(result_1->get(8), 16);
  EXPECT_EQ(result_2->get(2), 24);
  EXPECT_EQ(result_2->get(4), 28);
  EXPECT_EQ(result_2->get(6), 32);
  EXPECT_EQ(result_2->get(8), 36);
}

}  // namespace blender::fn::tests