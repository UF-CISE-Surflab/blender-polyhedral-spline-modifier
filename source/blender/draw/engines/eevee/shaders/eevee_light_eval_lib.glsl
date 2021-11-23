
/**
 * This is an eval function that needs to be added after main fragment shader.
 * A prototype needs to be declared before main in order to use it.
 *
 * The resources expected to be defined are:
 * - lights
 * - lights_zbins
 * - light_culling
 * - lights_culling_words
 * - shadows
 * - shadow_atlas_tx
 * - shadow_tilemaps_tx
 * - sss_transmittance_tx
 * - utility_tx
 *
 * All of this is needed to avoid using macros and performance issues with large
 * arrays as function arguments.
 */

#pragma BLENDER_REQUIRE(eevee_light_lib.glsl)

void light_eval(ClosureDiffuse diffuse,
                ClosureReflection reflection,
                vec3 P,
                vec3 V,
                float vP_z,
                float thickness,
                inout vec3 out_diffuse,
                inout vec3 out_specular)
{
  vec2 uv = vec2(reflection.roughness, safe_sqrt(1.0 - dot(reflection.N, V)));
  uv = uv * UTIL_TEX_UV_SCALE + UTIL_TEX_UV_BIAS;
  vec4 ltc_mat = utility_tx_sample(uv, UTIL_LTC_MAT_LAYER);
  float ltc_mag = utility_tx_sample(uv, UTIL_LTC_MAG_LAYER).x;

  ITEM_FOREACH_BEGIN (light_culling, lights_zbins, lights_culling_words, vP_z, l_idx) {
    LightData light = lights[l_idx];
    vec3 L;
    float dist;
    light_vector_get(light, P, L, dist);

    float visibility = light_attenuation(light, L, dist);

    if ((light.shadow_id != LIGHT_NO_SHADOW) && (visibility > 0.0)) {
      vec3 lL = light_world_to_local(light, -L) * dist;

      float shadow_delta = shadow_delta_get(
          shadow_atlas_tx, shadow_tilemaps_tx, light, shadows[l_idx], lL, dist, P);

      /* Transmittance evaluation first to use initial visibility. */
      if (diffuse.sss_id != 0u && light.diffuse_power > 0.0) {
        float delta = max(thickness, shadow_delta);

        vec3 intensity =
            visibility * light.transmit_power *
            light_translucent(
                sss_transmittance_tx, light, diffuse.N, L, dist, diffuse.sss_radius, delta);
        out_diffuse += light.color * intensity;
      }

      visibility *= float(shadow_delta - shadows[l_idx].bias <= 0.0);
    }

    if (visibility < 1e-6) {
      continue;
    }

    if (light.diffuse_power > 0.0) {
      float intensity = visibility * light.diffuse_power *
                        light_diffuse(utility_tx, light, diffuse.N, V, L, dist);
      out_diffuse += light.color * intensity;
    }

    if (light.specular_power > 0.0) {
      float intensity = visibility * light.specular_power *
                        light_ltc(utility_tx, light, reflection.N, V, L, dist, ltc_mat);
      out_specular += light.color * intensity;
    }
  }
  ITEM_FOREACH_END
}