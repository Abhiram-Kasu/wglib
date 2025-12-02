struct VertexInput {
    @location(0) position: vec3<f32>,
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
    output.position = vec4f(model.position, 1.0);
    output.color = model.color;
    return output;

}

@fragment
fn fragmentMain(input: VertexOutput) -> @location(0) vec4f {
    return vec4f(input.color, 1.0);
}
