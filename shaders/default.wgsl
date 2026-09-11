struct VertexInput {
    @location(0) position: vec2<f32>,
    @location(1) color: vec3<f32>,
}

;

struct VertexOutput {
    @builtin(position) position: vec4<f32>,
    @location(0) color: vec3<f32>,
}

;

struct Uniforms {
    @location(0) dimensions: vec2<f32>,
}

@group(0) @binding(0) var<uniform> uniforms: Uniforms;

@vertex
fn vertexMain(model: VertexInput) -> VertexOutput {
    var output: VertexOutput;
    // Convert from screen space (0,0 at top-left) to NDC (-1,-1 at bottom-left, 1,1 at top-right)
    // Screen space: x: [0, width], y: [0, height]
    // NDC: x: [-1, 1], y: [-1, 1]
    let ndc_x = (model.position.x / uniforms.dimensions.x) * 2.0 - 1.0;
    let ndc_y = 1.0 - (model.position.y / uniforms.dimensions.y) * 2.0;
    output.position = vec4f(ndc_x, ndc_y, 0.0, 1.0);
    output.color = model.color;
    return output;

}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4f {
    return vec4f(input.color, 1.0);
}
