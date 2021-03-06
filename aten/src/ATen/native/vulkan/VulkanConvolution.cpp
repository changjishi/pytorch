#include <ATen/native/utils/ParamUtils.h>

#include <ATen/native/vulkan/VulkanAten.h>
#include <ATen/native/vulkan/VulkanCommon.h>
#include <ATen/native/vulkan/VulkanConvolution.h>

namespace at {
namespace native {
namespace vulkan {
namespace detail {
namespace convolution2d {

namespace {
bool available(
    const Tensor& weight,
    const c10::optional<Tensor>& bias,
    const IntArrayRef padding,
    const IntArrayRef stride,
    const IntArrayRef dilation,
    const int64_t groups,
    const float output_min,
    const float output_max) {
  return at::native::is_vulkan_available() && (4 == weight.ndimension()) &&
      (at::Backend::CPU == weight.options().backend()) &&
      (kFloat == weight.scalar_type());
}

} // namespace

c10::intrusive_ptr<vulkan::Conv2dOpContext> createConv2dClampPrePackOpContext(
    Tensor weight,
    c10::optional<Tensor> bias,
    std::vector<int64_t> stride,
    std::vector<int64_t> padding,
    std::vector<int64_t> dilation,
    int64_t groups,
    c10::optional<Scalar> output_min,
    c10::optional<Scalar> output_max) {
  return vulkan::VulkanConv2dOpContext::create_context(
      std::move(weight),
      std::move(bias),
      std::move(padding),
      std::move(stride),
      std::move(dilation),
      groups,
      output_min,
      output_max);
}

Tensor Conv2dClampRun::operator()(
    const Tensor& input,
    const c10::intrusive_ptr<vulkan::Conv2dOpContext>& op_context) {
  return op_context->run(input);
}

ContextConv2D create(
    const Tensor& weight,
    const c10::optional<Tensor>& bias,
    const IntArrayRef padding,
    const IntArrayRef stride,
    const IntArrayRef dilation,
    const int64_t groups,
    const float output_min,
    const float output_max) {
  const auto padding_expanded = expand_param_if_needed(padding, "padding", 2);
  const auto stride_expanded = expand_param_if_needed(stride, "stride", 2);
  const auto dilation_expanded =
      expand_param_if_needed(dilation, "dilation", 2);
  const Tensor weight_nchw = weight.contiguous();
  return ContextConv2D{
      at::native::vulkan_convolution_prepack_weights(weight),
      bias.has_value() ? c10::make_optional((*bias).vulkan()) : c10::nullopt,
      {weight_nchw.sizes()[0],
       weight_nchw.sizes()[1],
       weight_nchw.sizes()[2],
       weight_nchw.sizes()[3]},
      {padding_expanded[0], padding_expanded[1]},
      {stride_expanded[0], stride_expanded[1]},
      {dilation_expanded[0], dilation_expanded[1]},
      groups};
}

Tensor run(const ContextConv2D& context, const Tensor& input) {
  return at::native::vulkan_convolution_prepacked(
      input,
      context.weight_size_,
      context.weight_prepacked_vulkan_,
      context.bias_vulkan_,
      context.padding_,
      context.stride_,
      context.dilation_,
      context.groups_);
}

} // namespace convolution2d
} // namespace detail
} // namespace vulkan
} // namespace native
} // namespace at
