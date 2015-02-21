//#define SHADERTOY

// !!! I have given up on this approach, since I can't figure out how to use it
// to draw rounded borders when the border widths are different. For example, 
// left border is 5 pixels, and top border is 10 pixels, with a radius of 5.
// Also, I don't know how to construct an implicit function for an ellipse
// that produces a correctly antialiased result. The problem is that the
// distance function for an ellipse ends up biasing you so that the shorter
// axis is much "tighter".
// Elliptical arc rendering is necessary to cover non-uniform border radii,
// as specified in http://www.w3.org/TR/css3-background/#corners.
// I'm going to try cubic spline rendering instead.

#ifdef SHADERTOY
    #define TOP_LEFT 0.0
    #define TOP_RIGHT 1.0
    #define BOTTOM_LEFT 2.0
    #define BOTTOM_RIGHT 3.0

    vec4 premultiply(vec4 c)
    {
        return vec4(c.r * c.a, c.g * c.a, c.b * c.a, c.a);
    }
#else
    uniform vec2    vport_hsize;
    uniform vec2    out_vector;
    uniform vec2    shadow_offset;
    uniform vec4    shadow_color;
    uniform float   shadow_size_inv;
    uniform vec2    edges;
    varying vec4    pos;
    varying vec4    color;
    varying float   radius;
    varying float   border_width;
    varying vec4    border_color;

    vec2 to_screen(vec2 unit_pt)
    {
        return (vec2(unit_pt.x, -unit_pt.y) + vec2(1,1)) * vport_hsize;
    }
#endif

vec2 clamp_to_edges(vec2 pos, vec2 edges, vec2 out_vector)
{
    // Clamp our distance from the outside edge to [-inf, 0],
    // then add that distance back to the edge.
    vec2 distance_outside = (pos - edges) * out_vector;
    return edges + out_vector * min(distance_outside, vec2(0.0, 0.0));
}

// Our boxes lie on integer coordinates. For example, a box that has perfectly sharp edges
// will have coordinates (5,5,10,10). However, fragment coordinates here are the centers
// of pixels, so the origin fragment lies at (0.5,0.5).
// In order to calculate blending factors, we first clamp the fragment location to our
// rectangle's box. Then, we measure the distance from the clamped location to the original
// location. A distance of 0 means the fragment is completely inside the box.
// Because our boxes lie on integer coordinates, but our fragment centers are at 0.5 offsets,
// we need to shrink our box by 0.5 on all sides, so that a fragment on the edge is at
// a distance of exactly 0, and any fragment outside that has an increasing distance.

// I suspect it is going to be better to have a single shader that gets issued on 4 primitives
// (ie 4 corners of rectangle) instead of a single invocation. But HOW do we communicate
// this...
void main()
{
#ifdef SHADERTOY
    vec4 color = vec4(0.0, 0.5, 0.0, 1.0);
    vec4 border_color = vec4(0.5, 0.0, 0.0, 1.0);
    float border_width = 35.0;
    vec2 edges = vec2(124.0, 124.0);
    
    float type = TOP_LEFT;
    
    // screen pos
    vec2 spos = vec2(gl_FragCoord.x, iResolution.y - gl_FragCoord.y);
   
    float radius = 35.0;
    float shadow_size = 0.0;
    float shadow_size_inv = 0.0;
    if (shadow_size != 0.0)
        shadow_size_inv = 1.0 / shadow_size;
    vec4 shadow_color = vec4(0.0, 0.0, 0.0, 0.5);
    vec2 shadow_offset = vec2(1.0, 1.0);

    // The 'out vector' points outward from our box's circle.
    vec2 out_vector;
    if (type == TOP_LEFT)           { out_vector = vec2(-1.0, -1.0); }
    else if (type == TOP_RIGHT)     { out_vector = vec2( 1.0, -1.0); }
    else if (type == BOTTOM_LEFT)   { out_vector = vec2(-1.0,  1.0); }
    else if (type == BOTTOM_RIGHT)  { out_vector = vec2( 1.0,  1.0); }
#else
    vec2 spos = to_screen(pos.xy);
#endif
  
    // Fragment samples are at pixel centers, so we have no choice but
    // to shrink our edges back by half a pixel. If we don't do this,
    // then the pixels immediately outside our border end up being a
    // distance of 0.5 from our edge, which gives it some color. We need
    // any pixels outside of our box to be completely unlit.
    
    float border_shrink = max(border_width, radius) + 0.5;
  
    vec2 edges_circle = edges - (0.5 + radius) * out_vector;
    vec2 edges_border = edges - border_shrink * out_vector;
    vec2 edges_shadow_circle = edges_circle + shadow_offset;
    
    vec2 pos_circle = clamp_to_edges(spos, edges_circle, out_vector);
    vec2 pos_border = clamp_to_edges(spos, edges_border, out_vector);
    vec2 pos_shadow_circle = clamp_to_edges(spos, edges_shadow_circle, out_vector);
    
    float dist_circle = length(spos - pos_circle);
    float dist_border = length(spos - pos_border);
    
    vec4 shadow_out = vec4(0,0,0,0);
    if (shadow_size_inv != 0.0)
    {
        float dist_shadow_circle = length(spos - pos_shadow_circle);
        float shdist = (dist_shadow_circle - radius - 1.0) * shadow_size_inv;
        shdist = clamp(shdist, 0.0, 1.0);
        // cheap gaussian approximation (1 - x^2)^2 - I'm definitely not satisfied with this!
        float shadow_strength = 1.0 - shdist * shdist;
        shadow_strength *= shadow_strength;
        shadow_out = mix(shadow_out, premultiply(shadow_color), shadow_strength);
    }

    vec4 outcolor = premultiply(color);
    
    // blend border onto outcolor
    float border_blend = dist_border - (border_shrink - border_width) + 0.5;
    border_blend = clamp(border_blend, 0.0, 1.0);
    vec4 border_out = premultiply(border_color);
    outcolor = mix(outcolor, border_out, border_blend);
    
    float bgblend = 1.0 + radius - dist_circle;
    outcolor *= clamp(bgblend, 0.0, 1.0);
    
    // blend outcolor onto shadow_out
    outcolor = (1.0 - outcolor.a) * shadow_out + outcolor;
    
#ifdef SHADERTOY
    // basecolor is for shadertoy
    vec4 basecolor = vec4(1,1,1,1);
    outcolor = (1.0 - outcolor.a) * basecolor + outcolor;

    float igamma = 1.0/2.2;
    gl_FragColor.rgb = pow(outcolor.rgb, vec3(igamma, igamma, igamma));
    gl_FragColor.a = outcolor.a;
#else
    #ifdef XO_SRGB_FRAMEBUFFER
        gl_FragColor = outcolor;
    #else
        float igamma = 1.0/2.2;
        gl_FragColor.rgb = pow(outcolor.rgb, vec3(igamma, igamma, igamma));
        gl_FragColor.a = outcolor.a;
    #endif
#endif
}
