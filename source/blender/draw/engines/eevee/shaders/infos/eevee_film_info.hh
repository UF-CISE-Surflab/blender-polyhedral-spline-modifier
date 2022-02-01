
#include "gpu_shader_create_info.hh"

/* -------------------------------------------------------------------- */
/** \name Film Filter
 * \{ */

GPU_SHADER_CREATE_INFO(eevee_film_filter)
    .uniform_buf(0, "CameraData", "camera")
    .uniform_buf(1, "FilmData", "film")
    .sampler(0, ImageType::FLOAT_2D, "input_tx")
    .sampler(1, ImageType::FLOAT_2D, "data_tx")
    .sampler(2, ImageType::FLOAT_2D, "weight_tx")
    .fragment_out(0, Type::VEC4, "out_data")
    .fragment_out(1, Type::FLOAT, "out_weight")
    .typedef_source("eevee_shader_shared.hh")
    .fragment_source("eevee_film_filter_frag.glsl")
    .additional_info("draw_fullscreen");

GPU_SHADER_CREATE_INFO(eevee_film_filter_depth)
    .uniform_buf(0, "CameraData", "camera")
    .uniform_buf(1, "FilmData", "film")
    .sampler(0, ImageType::DEPTH_2D, "input_tx")
    .sampler(1, ImageType::FLOAT_2D, "data_tx")
    .sampler(2, ImageType::FLOAT_2D, "weight_tx")
    .fragment_out(0, Type::VEC4, "out_data")
    .fragment_out(1, Type::FLOAT, "out_weight")
    .typedef_source("eevee_shader_shared.hh")
    .fragment_source("eevee_film_filter_frag.glsl")
    .additional_info("draw_fullscreen");

/** \} */

/* -------------------------------------------------------------------- */
/** \name Film Resolve
 * \{ */

GPU_SHADER_CREATE_INFO(eevee_film_resolve)
    .uniform_buf(1, "FilmData", "film")
    .sampler(0, ImageType::FLOAT_2D, "data_tx")
    .sampler(1, ImageType::FLOAT_2D, "weight_tx")
    .sampler(2, ImageType::FLOAT_2D, "first_sample_tx")
    .fragment_out(0, Type::VEC4, "out_color")
    .typedef_source("eevee_shader_shared.hh")
    .fragment_source("eevee_film_resolve_frag.glsl")
    .additional_info("draw_fullscreen");

GPU_SHADER_CREATE_INFO(eevee_film_resolve_depth)
    .uniform_buf(1, "FilmData", "film")
    .sampler(0, ImageType::FLOAT_2D, "data_tx")
    .sampler(1, ImageType::FLOAT_2D, "weight_tx")
    //.fragment_out(0, Type::FLOAT, "gl_FragDepth")
    .typedef_source("eevee_shader_shared.hh")
    .fragment_source("eevee_film_resolve_depth_frag.glsl")
    .additional_info("draw_fullscreen");

/** \} */