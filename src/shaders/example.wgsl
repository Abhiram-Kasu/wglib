struct Uniforms{
    multiplier: vec4<f32>
};

@group(0)
@binding(0)
var<uniform> mul: Uniforms;

@group(0) @binding(1) var<storage, read> data_buffer: array<f32>;
@group(0) @binding(2) var<storage, read_write> resul_buffer: array<f32>;

@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let index = global_id.x;
    if(index < arrayLength(&data_buffer)){
        resul_buffer[index] = data_buffer[index] * mul.multiplier.x;
    }
}
