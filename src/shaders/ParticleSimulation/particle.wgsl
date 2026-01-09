struct Particle {
    velocity: vec2<f32>,
    position: vec2<f32>,
    radius: f32
};

struct Uniforms {
  color: vec4<f32>,
  size: vec2<u32>,
  dt: f32,
  gravity: f32,
  damping: f32,
  forceAmplitude: f32,
  decayLength: f32
};



@group(0) @binding(0) var<storage, read> input_buffer: array<Particle>;
@group(0) @binding(1) var<storage, read_write> output_buffer: array<Particle>;
@group(0) @binding(2) var<uniform> uniforms: Uniforms;
@group(0) @binding(3) var output_texture: texture_storage_2d<rgba8unorm, write>;


@compute @workgroup_size(64)
fn main(@builtin(global_invocation_id) global_id: vec3<u32>) {

    let idx = global_id.x;

    if (idx >= arrayLength(&input_buffer)) {
        return;
    }

    let current = input_buffer[idx];
    var position = current.position;
    var velocity = current.velocity;

    var accumulatedForce = vec2<f32>(0.0, 0.0);

    // Gravity
    accumulatedForce.y += uniforms.gravity;

    // Particle repulsion
    for(var i: u32 = 0; i < arrayLength(&input_buffer); i++) {
        if(i == idx) {
            continue;
        }

        let other = input_buffer[i];
        let delta = position - other.position;
        let distance = length(delta);

        if (distance < 0.001) {
            continue;
        }

        let combinedRadius = current.radius + other.radius;

        if (distance < combinedRadius * 2.0) {
            let direction = delta / distance;
            let forceMagnitude = uniforms.forceAmplitude * exp(-distance / uniforms.decayLength);
            accumulatedForce += direction * forceMagnitude;
        }
    }

    let acceleration = accumulatedForce;

    // Update velocity
    velocity += acceleration * uniforms.dt;
    velocity *= uniforms.damping;

    // Update position
    position += velocity * uniforms.dt;

    // Wall collisions
    let bounds = vec2<f32>(f32(uniforms.size.x), f32(uniforms.size.y));

    if (position.x - current.radius < 0.0) {
        position.x = current.radius;
        velocity.x = abs(velocity.x) * 0.8;
    }

    if (position.x + current.radius > bounds.x) {
        position.x = bounds.x - current.radius;
        velocity.x = -abs(velocity.x) * 0.8;
    }

    if (position.y - current.radius < 0.0) {
        position.y = current.radius;
        velocity.y = abs(velocity.y) * 0.8;
    }

    if (position.y + current.radius > bounds.y) {
        position.y = bounds.y - current.radius;
        velocity.y = -abs(velocity.y) * 0.8;
    }

    // Write output
    output_buffer[idx].position = position;
    output_buffer[idx].velocity = velocity;
    output_buffer[idx].radius = current.radius;

    // Draw particle
    let center = vec2<i32>(i32(position.x), i32(position.y));
    let radius_i = i32(current.radius);

    for (var dy: i32 = -radius_i; dy <= radius_i; dy++) {
        for (var dx: i32 = -radius_i; dx <= radius_i; dx++) {
            let dist_sq = dx * dx + dy * dy;
            let radius_sq = radius_i * radius_i;

            if (dist_sq <= radius_sq) {
                let pixel_pos = center + vec2<i32>(dx, dy);

                if (pixel_pos.x >= 0 && pixel_pos.x < i32(uniforms.size.x) &&
                    pixel_pos.y >= 0 && pixel_pos.y < i32(uniforms.size.y)) {
                    textureStore(output_texture, pixel_pos, uniforms.color);
                }
            }
        }
    }
}
