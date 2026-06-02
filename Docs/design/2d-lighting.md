# Design: 2D dynamic lighting for Diorama sprites

Status: design (Tier-1 roadmap item, task #26). No implementation yet. This
document records the approach, the Atom integration decision behind it, and a
phased plan, so the build is fast and low-risk once verification is possible.

## Goal

Let sprites (and tilemap tiles) take **real dynamic scene lights**: a sun
(directional) and a handful of moving point/spot lights, modulated by a per-sprite
**normal map** so flat art gets shape and the scene reacts to lights moving
through it. This is the headline differentiator: it is the look pure-2D engines
reach for awkwardly, and Diorama gets it because it already renders through Atom.

Non-goals for v1: shadows cast by lights (we already have soft ground-contact
shadows, a separate feature), area lights, light cookies, global illumination.

## How Atom delivers lights, and the decision

Atom has three ways a shader can see scene lights. We evaluated all three against
our renderer, which is a `DynamicDrawContext` (RPI) drawing batched sprite quads
with a custom shader.

### Option A: full Forward+ tiled culling (rejected)

The standard PBR path reads a per-tile light list (`LightCullingTileIterator`,
`m_tileLightData` + `m_lightListRemapped`) bound in the **PassSrg** of the forward
pass. A `DynamicDrawContext` does **not** get the PassSrg, so the tile iterator is
unreachable from our draw unless we draw from inside the forward pass or stand up a
custom culling pass. That is a large amount of pipeline machinery for a feature
whose realistic light count is small. Rejected: too heavy, and it couples us to a
specific pass layout.

Reference: `Gems/Atom/Feature/Common/Assets/ShaderLib/Atom/Features/LightCulling/LightCullingTileIterator.azsli`,
`.../Pipeline/Forward/ForwardPassSrg.azsli`.

### Option B: read the light SRGs directly from the shader (partial)

Directional lights live in **SceneSrg** (`m_directionalLights`,
`m_directionalLightCount`) and need no culling: the standard directional path just
loops the count. Simple point/spot lights live in **ViewSrg**, but the non-culled
fallback loops a CPU-pre-culled visible-index list
(`m_visibleSimplePointLightIndices`) that is populated for the forward pass, not
guaranteed for an arbitrary dynamic draw. ViewSrg is definitely bound for our
draw (our current shader already reads `ViewSrg::m_viewProjectionMatrix`); whether
SceneSrg and the visible-light lists are populated for a dynamic-draw item is
**not guaranteed** and would need on-GPU verification. Usable for the sun;
unreliable for point lights.

References: `.../ShaderResourceGroups/CoreLights/SceneSrg.azsli`,
`.../CoreLights/ViewSrg.azsli`,
`.../Pipeline/Forward/ForwardPassDirectionalLights.azsli`.

### Option C: gather lights on the CPU into our own SRG (chosen)

The feature processor queries the light feature processors on the CPU each frame,
selects a bounded set of lights, and packs them into **our own constant buffer** in
a Diorama-owned SRG. The shader loops that small fixed array. This:

- depends on **nothing** outside our own renderer (no PassSrg, no assumptions about
  which Atom SRGs are bound to a dynamic draw), so it is robust and verifiable in
  isolation,
- avoids all culling machinery,
- fits 2D perfectly (a scene is lit by a few lights, not hundreds),
- gives us full control over which lights affect the sprite layer.

CPU interfaces to query (all in `Gems/Atom/Feature/Common/Code/Include/Atom/Feature/CoreLights/`):

- `DirectionalLightFeatureProcessorInterface` - the sun(s): direction, rgb intensity.
- `SimplePointLightFeatureProcessorInterface` - `GetLightCount()` and a light
  buffer of `SimplePointLight { float3 m_position; float m_invAttenuationRadiusSquared; float3 m_rgbIntensityCandelas; }`.
- `SimpleSpotLightFeatureProcessorInterface` - same shape plus direction and cone
  cosines (v2).

Light data structs: `.../ShaderLib/Atom/Features/PBR/Lights/LightStructures.azsli`.

**Chosen: Option C.** Start with directional + simple point lights.

## Data model

A Diorama lighting SRG (constant buffer), filled once per frame, bound on every
sprite batch draw (the same handful of lights light the whole sprite layer in v1,
which is the normal 2D model):

```hlsl
struct DioramaLight
{
    float3 m_position;      // world; for directional, this is -direction * large
    float  m_invRadiusSq;   // 0 for directional (no attenuation)
    float3 m_rgbIntensity;  // linear color * intensity
    float  m_isDirectional; // 1 = directional, 0 = point
};

ShaderResourceGroup DioramaLightSrg : SRG_PerDraw   // or a per-context CB
{
    float3 m_ambient;       // flat fill so unlit faces are not pure black
    uint   m_lightCount;    // clamped to DIORAMA_MAX_LIGHTS
    DioramaLight m_lights[DIORAMA_MAX_LIGHTS]; // DIORAMA_MAX_LIGHTS = 8 for v1
};
```

Sprite config gains an optional **normal map** texture slot (a second `Texture2D`
in the existing per-draw SRG). When absent, the shader uses the quad's geometric
normal (flat lighting); when present, it perturbs per-texel.

## Shader changes (DioramaSprite.azsl)

The quad is flat, so its tangent basis is exactly the basis the CPU already builds
in `AppendQuad`: tangent = `right`, bitangent = `up`, normal = `right x up` (the
face direction; toward the camera for a billboard). We pass that basis to the
pixel stage (or reconstruct it), sample the normal map in tangent space, transform
to world, then accumulate simple Lambertian diffuse:

```
N = normalize( normalMap.xyz mapped to [-1,1], transformed by (T,B,N) basis )
color = ambient
for i in 0..m_lightCount:
    L, atten = directional ? (-dir, 1) : (normalize(Lpos - Pworld), falloff(invRadiusSq))
    color += m_rgbIntensity[i] * atten * saturate(dot(N, L))
out = albedo.rgb * color   (albedo from the existing texture * vertex tint)
```

The vertex stage must additionally output world position and the TBN basis. We can
reuse Atom's `DirectionalLightUtil`/`SimplePointLightUtil` math for parity, or keep
a self-contained Lambert term for v1 simplicity (PBR specular is overkill for flat
sprites). Decision: self-contained Lambert + ambient for v1; revisit PBR parity if
the look needs it.

## Feature-processor changes (SpriteFeatureProcessor)

- In `Activate`/`Render`, resolve the light feature processors from the parent
  scene (cache the interface pointers; they are scene-scoped).
- Once per `Render` (not per sprite), gather: read directional light(s) and up to
  `DIORAMA_MAX_LIGHTS` point lights into a reused scratch array (no per-frame heap
  allocation, matching the existing render-loop rule). v1 selection: first N; v2:
  nearest-N to the sprite-layer centroid or per-batch.
- Pack the scratch array into the lighting SRG (new SRG, compiled once per frame)
  and bind it alongside the texture SRG on each batch `DrawIndexed`.
- Normal map: add `m_normalMap` to `SpriteComponentConfig`; bind it (or a flat
  default `(0.5,0.5,1)` normal) into the per-draw SRG.

## Console variables

- `r_dioramaSpriteLighting` (default 1): master on/off. Off keeps today's unlit
  `albedo * tint` path exactly, so it is a clean fallback.
- `r_dioramaSpriteAmbient` (default ~0.35): ambient fill level.
- Reuse the existing pattern (`AZ_CVAR`, see depth-sort/shadow CVars).

## Security and performance

- `m_lightCount` is **clamped** to `DIORAMA_MAX_LIGHTS` on the CPU before it ever
  sizes a loop; the array is fixed-size, so no asset/scene value sizes a GPU buffer
  (consistent with the VISION security criterion).
- Light gather is O(lights) once per frame, reusing scratch; no per-frame heap
  allocation in the render loop.
- Normal map is an ordinary streaming image: validated by the existing image
  pipeline; a missing/!ready asset falls back to the flat default normal, never a
  broken bind.
- Batching is unchanged: lighting is global per frame in v1, so it does not split
  batches. (Per-sprite light selection in v2 would, if done per draw, need either a
  per-instance light index or acceptance of the global set; keep v1 global.)

## Phasing

1. **v1**: directional (sun) + ambient + per-sprite normal map, global light set,
   self-contained Lambert. The full "wow" with the least risk. CVar-gated.
2. **v2**: simple point lights (moving), still a global set; nearest-N selection.
3. **v3**: spot lights; optional per-batch light selection; emissive/self-illum
   channel for glowing sprites (pairs with the post-processing bloom roadmap item).

## Verification plan (needs the monitor)

- A scene with one sprite + a normal map + a single rotating point light: the
  shading hotspot must track the light around the sprite.
- Toggle `r_dioramaSpriteLighting 0/1`: must fall back to the exact current look.
- A normal-mapped floor tile under a moving light: relief should appear/track.
- Confirm no batch-count regression (lighting must not split batches in v1).
- Confirm SceneSrg-independence: lighting works without relying on any Atom pass
  SRG being bound to our dynamic draw.

## Open questions

- Does `SimplePointLightFeatureProcessorInterface::GetLightBuffer()` give a
  CPU-readable view, or must we read positions via the feature-processor's own
  accessors? If only a GPU buffer, gather from the light data we set, or add a thin
  CPU mirror. (Resolve at implementation: inspect the concrete feature processor,
  not just the interface.)
- Billboard normal mapping: since a billboard always faces the camera, tangent-space
  normal mapping lights it as if facing the viewer; confirm that reads as intended
  for upright billboards vs. a fixed world normal. Cheap to try both behind the CVar.
