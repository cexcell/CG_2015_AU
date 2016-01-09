#version 330 core
in vec2 coord;
out vec4 color;

uniform vec3 coords_zoom;
uniform vec2 limits;
uniform sampler1D myTexture;

void main()
{
	vec4 frag_color;
	float max_iters = limits.x;
	float radius = limits.y;
	float zoom = coords_zoom.z;
	float x_c = coords_zoom.x;
	float y_c = coords_zoom.y;
	float init_re = x_c + (gl_FragCoord.x / 800.0) * zoom;
	float init_im = y_c + (1.0 - gl_FragCoord.y / 600.0) * zoom;
	float re, im;
	re = im = 0.0f;
	float iters = 0.0f;
	float r2 = 0;
	while (r2 < radius && iters < max_iters)
	{
	    float newRe = re * re - im * im + init_re;
	    im = 2.0 * re * im + init_im;
	    re = newRe;
	    iters += 1.0;
	    r2 = re * re + im * im;
	}

	if (iters >= max_iters - 0.1) {
	    frag_color = vec4(0.0, 0.0, 0.0, 1.0);
	}
	else {
	    frag_color = texture(myTexture, iters / max_iters).rbga;
	}

	gl_FragColor = frag_color;
}