@group(0) @binding(0) var<storage, read> input: array<u32>;
@group(0) @binding(1) var<storage, read_write> output: array<u32>;
@group(0) @binding(2) var output_texture: texture_storage_2d<rgba8unorm, write>;

struct Uniforms {
    width: u32,
    height: u32,
}

@group(0) @binding(3) var<uniform> uniforms: Uniforms;

fn get_index(x: u32, y: u32) -> u32 {
    return y * uniforms.width + x;
}

fn is_alive(x: u32, y: u32) -> u32 {
    if (x >= uniforms.width || y >= uniforms.height) {
        return 0u;
    }
    let index = get_index(x, y);
    return input[index];
}

fn count_neighbors(x: u32, y: u32) -> u32 {
    var count = 0u;

    for (var dy = -1i; dy <= 1i; dy++) {
        for (var dx = -1i; dx <= 1i; dx++) {
            if (dx == 0i && dy == 0i) {
                continue;
            }

            let nx = i32(x) + dx;
            let ny = i32(y) + dy;

            if (nx >= 0i && ny >= 0i) {
                count += is_alive(u32(nx), u32(ny));
            }
        }
    }

    return count;
}

@compute @workgroup_size(8, 8)
fn conways_game_of_life_and_write(@builtin(global_invocation_id) global_id: vec3<u32>) {
    let x = global_id.x;
    let y = global_id.y;

    if (x >= uniforms.width || y >= uniforms.height) {
        return;
    }

    let index = get_index(x, y);
    let current_state = input[index];
    let neighbor_count = count_neighbors(x, y);

    var new_state = 0u;


    if (current_state == 1u) {

        if (neighbor_count == 2u || neighbor_count == 3u) {
            new_state = 1u;
        }
    } else {

        if (neighbor_count == 3u) {
            new_state = 1u;
        }
    }


    output[index] = new_state;

    let color = f32(new_state);
    textureStore(output_texture, vec2<u32>(x, y), vec4<f32>(color, color, color, 1.0));
}
