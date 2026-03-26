---
status: verifying
trigger: "CLIP vision encoder fails with GGML_ASSERT at clip.hpp:659 - pixel_values tensor dimensions mismatch"
created: 2026-03-26T00:00:00Z
updated: 2026-03-26T00:00:00Z
symptoms_prefilled: true
goal: find_and_fix
---

## Current Focus

hypothesis: Input image dimensions don't match CLIP's expected 224x224 size
test: Added image resizing to 224x224 before CLIP processing in both TI2V and I2V
expecting: CLIP assertion will pass with resized images
next_action: Verify fix resolves the assertion failure

## Symptoms

expected: CLIP vision encoder processes input image successfully
actual: GGML_ASSERT fails at clip.hpp:659 with dimension mismatch
errors: "GGML_ASSERT(pixel_values->ne[0] == image_size && pixel_values->ne[1] == image_size && pixel_values->ne[2] == num_channels)"
reproduction: Call TI2V or I2V API with image input
started: Unknown - investigating

## Eliminated

(none yet)

## Evidence

- timestamp: 2026-03-26
  checked: CLIPVisionEmbeddings::forward() assertion at clip.hpp:659
  found: Assertion expects pixel_values->ne[0] == image_size (224), ne[1] == image_size (224), ne[2] == num_channels (3)
  implication: Tensor dimensions must be [image_size, image_size, num_channels, batch_size]

- timestamp: 2026-03-26
  checked: sd_image_to_ggml_tensor() function in ggml_extend.hpp:432-444
  found: Function validates tensor->ne[0] == image.width, ne[1] == image.height, ne[2] == image.channel, ne[3] == 1
  implication: Tensor is created with dimensions [width, height, channels, 1]

- timestamp: 2026-03-26
  checked: pixel_values tensor creation in wan-api.cpp:1029-1031 (TI2V) and 1338-1340 (I2V)
  found: Both create tensor as ggml_new_tensor_4d(ctx, GGML_TYPE_F32, sd_img.width, sd_img.height, sd_img.channel, 1)
  implication: Tensor created with dimensions [width, height, channels, 1] - CORRECT format

- timestamp: 2026-03-26
  checked: Image preprocessing - does image get resized before CLIP?
  found: No resize operation found before pixel_values creation in either TI2V or I2V
  implication: Input image dimensions must match CLIP's expected 224x224, but code doesn't enforce this

## Resolution

root_cause: Input image is not resized to 224x224 before being passed to CLIP. The assertion fails because image dimensions don't match the expected image_size (224). The tensor format [width, height, channels, 1] is correct, but the actual width/height values are not 224.

fix: Added image resizing to 224x224 before creating pixel_values tensor in both TI2V and I2V APIs using stb_image_resize

verification: (pending)
files_changed: [src/api/wan-api.cpp]
