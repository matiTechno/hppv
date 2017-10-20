const char* lines  = R"(

// author: @aadebdeb (https://twitter.com/aa_debdeb)

#ifdef GL_ES
precision mediump float;
#endif

#extension GL_OES_standard_derivatives : enable

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;


float line(vec2 p, vec2 p1, vec2 p2, float w) {
	float a = p1.y - p2.y;
	float b = -p1.x + p2.x;
	float c = p1.x * p2.y - p2.x * p1.y;
	
	float d = abs(a * p.x + b * p.y + c) / sqrt(pow(a, 2.0) + pow(b, 2.0));
	
	return smoothstep(0.0, d, w);
}

void main( void ) {
	
	vec2 st = gl_FragCoord.xy /min(resolution.x, resolution.y) * 2.0 - resolution / min(resolution.x, resolution.y);

	vec3 color = vec3(0.05);
	for (int i = 1; i <= 10; i++) {
		vec2 p1 = vec2(sin(sin(time * 0.269 * float(i) + float(i)) / float(i) * 1.95), -1.0);
		vec2 p2 = vec2(sin(sin(time * 0.363 * float(i) + float(i)) / float(i) * 2.03), 1.0);
		vec3 c = vec3(0.0);
		c += vec3(1.0, 0.0, 0.0) * line(st, p1, p2, 0.005);
		c += vec3(0.0, 1.0, 0.0) * line(st, p1, p2, 0.01);
		c += vec3(0.0, 0.0, 1.0) * line(st, p1, p2, 0.03);
		color.r = c.r > color.r ? c.r : color.r;
		color.g = c.g > color.g ? c.g : color.g;
		color.b = c.b > color.b ? c.b : color.b;
	}
	
	gl_FragColor = vec4(color, 1.0);
}

)";

const char* tunnel = R"(

#ifdef GL_ES
precision mediump float;
#endif

#extension GL_OES_standard_derivatives : enable

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

float iGlobalTime = time;
vec2 iResolution = resolution;

// Converted by Batblaster, this shader in his original form use textures and are removed because not supported.

/*
//Dain: this is mostly Shane's Cellular Tile Tunnel:  https://www.shadertoy.com/view/MscSDB

//I marked the stuff I changed with a comment followed by my name

    Cellular Tiled Tunnel
    ---------------------
    
    I've always liked the look of a 2nd order Voronoi surface. IQ's Leizex demo is a great
	prerendered example, and for anyone who can remember, Tomasz Dobrowolski's Suboceanic
	was cutting edge back in the day.

	Anyway, I've always wanted to get a proper example working in a shader... Yeah, I need
	bigger dreams. :) Unfortunately, I kind of realized that it wasn't going to be possible 
	until GPUs become even faster than they already are, so I figured I'd try the next best 
	thing and come up with a way to emulate the look with something cheap. This is the 
	result. It's not perfect, but it looks surprisingly similar.

	The regular 2nd order Voronoi algorithm involves a "lot" of operations. In general,
	27 cell checks - all involving a bunch of vector arithmetic, fract, sin, floor, 
	comparisons, etc... It's possible to cut down on cell checks, perform a bunch of
	optimizations, etc, but it's still too much work for a raymarcher.

	The surface here is produced via a repeat 3D tile approach. The look is achieved by 
	performing 2nd order distance checks on the tiles. I used a highly scientific approach
	which involved crossing my fingers, doing the distance checks and hoping for the best. :)
	Amazingly, it produced the result I was looking for.

	I covered the tile construction in other "cell tile" examples, so I'll spare you the 
	details, but it's pretty simple. The only additions here are the second order distance
	checks.

	In order to show the surface itself, I've made the example geometric looking - I hope
	you like brown, or whatever color that is. :) Note that individual cell regions are 
	colored	differently. I did that to show that it could be done, but I'm not convinced 
	that it adds to the aesthetics in any meaningful way.

	Anyway, I have a few more interesting examples that I'll put up pretty soon.
	
    Related examples: 

    Cellular Tiling - Shane
    https://www.shadertoy.com/view/4scXz2

	// For comparison, this example uses the standard 2nd order Voronoi algorithm. For fun,
	// I dropped the cell tile routine into it and it ran a lot faster.
	Voronoi - rocks - iq
	https://www.shadertoy.com/view/MsXGzM

	rgba leizex - Inigo Quilez
	http://www.pouet.net/prod.php?which=51829
	https://www.youtube.com/watch?v=eJBGj8ggCXU
	http://www.iquilezles.org/prods/index.htm

	Tomasz Dobrowolski - Suboceanic
	http://www.pouet.net/prod.php?which=18343

*/

#define PI 3.14159265358979
#define FAR 50. // Maximum allowable ray distance.

// Grey scale.
float getGrey(vec3 p){ return p.x*0.299 + p.y*0.587 + p.z*0.114; }

// Non-standard vec3-to-vec3 hash function.
vec3 hash33(vec3 p){ 
    
    float n = sin(dot(p, vec3(7, 157, 113)));    
    return fract(vec3(2097152, 262144, 32768)*n); 
}

// 2x2 matrix rotation.
mat2 rot2(float a){
    
    float c = cos(a); float s = sin(a);
	return mat2(c, s, -s, c);
}

// Tri-Planar blending function. Based on an old Nvidia tutorial.
vec3 tex3D( sampler2D tex, in vec3 p, in vec3 n ){
  
    n = max((abs(n) - 0.2)*7., 0.001); // max(abs(n), 0.001), etc.
    n /= (n.x + n.y + n.z );  
    
	vec3 tx = vec3(0.0,0.0,0.0);//(texture(tex, p.yz)*n.x + texture(tex, p.zx)*n.y + texture(tex, p.xy)*n.z).xyz;
    
    return tx*tx;
}


// The cellular tile routine. Draw a few objects (four spheres, in this case) using a minumum
// blend at various 3D locations on a cubic tile. Make the tile wrappable by ensuring the 
// objects wrap around the edges. That's it.
//
// Believe it or not, you can get away with as few as three spheres. If you sum the total 
// instruction count here, you'll see that it's way, way lower than 2nd order 3D Voronoi.
// Not requiring a hash function provides the biggest benefit, but there is also less setup.
// 
// The result isn't perfect, but 3D cellular tiles can enable you to put a Voronoi looking 
// surface layer on a lot of 3D objects for little cost.
//
//Dain: for alternative rocks
float sdBox( vec3 p, vec3 b )
{
  vec3 d = abs(p) - b;
  return min(max(d.x,max(d.y,d.z)),0.0) + length(max(d,0.0));
}

float drawSphere(in vec3 p){
    
    //Dain: warp space with sin so the rocks are of varying size
    p += sin(p);
    
    // Anything that wraps the domain will suffice, so any of the following will work.
    //p = cos(p*3.14159)*0.5; 
    //p = abs(cos(p*3.14159)*0.5);    
    p = fract(p)-.5;    
    
    //Dain: a few alternative rock styles
    return abs(p.x) + abs(p.y) + abs(p.z);   //big sharp rocks
	//return abs(sdBox(p, vec3(0.9, .10, 0.82))); //long jagged rocks
    
    return dot(p, p);
    
    // Other metrics to try.
    
    //p = abs(fract(p)-.5);
    //return dot(p, vec3(.5));
    
    //p = abs(fract(p)-.5);
    //return max(max(p.x, p.y), p.z);
    
    //p = cos(p*3.14159)*0.5; 
    //p = abs(cos(p*3.14159)*0.5);
    //p = abs(fract(p)-.5);
    //return max(max(p.x - p.y, p.y - p.z), p.z - p.x);
    //return min(min(p.x - p.y, p.y - p.z), p.z - p.x);
    
}
//Dain: alternative shape that makes big jagged rocks(not used)
float drawOct(in vec3 p){
   // p += sin(p);
    p = fract(p)-.5; 
    return abs(p.x) + abs(p.y) + abs(p.z);   //big sharp rocks
}
// Faster (I'm assuming), more streamlined version. See the comments below for an expanded explanation.
// The function below is pretty quick also, and can be expanded to include more spheres. This one
// takes advantage of the fact that only four object need sorting. With three spheres, it'd be even
// better.
float cellTile(in vec3 p){
  //  p *= 2.0;
    // Draw four overlapping objects (spheres, in this case) at various positions throughout the tile.
    vec4 v, d; 
    d.x = drawSphere(p - vec3(.81, .62, .53));
    p.xy = vec2(p.y-p.x, p.y + p.x)*.7071;
    d.y = drawSphere(p - vec3(.39, .2, .11));
    p.yz = vec2(p.z-p.y, p.z + p.y)*.7071;
    d.z = drawSphere(p - vec3(.62, .4, .06));
    p.xz = vec2(p.z-p.x, p.z + p.x)*.7071;
    d.w = drawSphere(p - vec3(.2, .82, .64));

    v.xy = min(d.xz, d.yw), v.z = min(max(d.x, d.y), max(d.z, d.w)), v.w = max(v.x, v.y); 
   
    //Dain: get 3 closest and use Fabrice's smooth voronoi 
    float m =  min(v.x, v.y);
    float m2 = min(v.z, v.w);
    float m3 = max(v.z, v.w);
    
    //Dain: if you want a more euclidean distance..
   // m = sqrt(m);
   // m2 = sqrt(m2);
   // m3 = sqrt(m3);
    
    //Dain: use Fabrice's formula to produce smooth result
    return min(2./(1./max(m2 - m, .001) + 1./max(m3 - m, .001)), 1.)*2.5;
    
  
}

/*
// Draw some spheres throughout a repeatable cubic tile. The offsets were partly based on 
// science, but for the most part, you could choose any combinations you want. Note the 
// normalized planar positional roation between sphere rendering to really mix things up. This 
// particular function is used by the raymarcher, so involves fewer spheres.
//
float cellTile(in vec3 p){

    // Storage for the closest distance metric, second closest and the current
    // distance for comparisson testing.
    //
    // Set the maximum possible value - dot(vec3(.5), vec3(.5)). I think my reasoning is
    // correct, but I have lousy deductive reasoning, so you may want to double check. :)
    vec3 d = (vec3(.75)); 
   
    
    // Draw some overlapping objects (spheres, in this case) at various positions on the tile.
    // Then do the fist and second order distance checks. Very simple.
    d.z = drawSphere(p - vec3(.81, .62, .53));
    d.x = min(d.x, d.z); //d.y = max(d.x, min(d.y, d.z)); // Not needed on the first iteration.
    
    p.xy = vec2(p.y-p.x, p.y + p.x)*.7071;
    d.z = drawSphere(p - vec3(.39, .2, .11));
    d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z);
    
    p.yz = vec2(p.z-p.y, p.z + p.y)*.7071;
    d.z = drawSphere(p - vec3(.62, .24, .06));
    d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z);
    
    p.xz = vec2(p.z-p.x, p.z + p.x)*.7071; 
    d.z = drawSphere(p - vec3(.2, .82, .64));
    d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z);

     
	// More spheres means better patterns, but slows things down.
    //p.xy = vec2(p.y-p.x, p.y + p.x)*.7071;
    //d.z = drawSphere(p - vec3(.48, .29, .2));
    //d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z);
    
    //p.yz = vec2(p.z-p.y, p.z + p.y)*.7071;
    //d.z = drawSphere(p - vec3(.06, .87, .78));
    //d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z); 
	

    
    // Returning what I'm hoping is a normalized result. Not that it
    // matters too much, but I'd like it normalized.
    // 2.66 seems to work, but I'll double check at some stage.
    // d.x: Minimum distance. Regular round Voronoi looking.
    // d.y - d.x - Maximum minus minimum, for that beveled Voronoi look.
    //
    return (d.y - d.x)*2.66; 
    //return 1. - d.x*2.66;
    //return 1. - sqrt(d.x)*1.63299; // etc.

    
}
*/

// Just like the function above, but used to return the regional cell ID...
// kind of. Either way, it's used to color individual raised sections in
// the same way that a regular Voronoi function can. It's only called once,
// so doesn't have to be particularly fast. It's kept separate to the
// raymarched version, because you don't want to be performing ID checks
// several times a frame when you don't have to. By the way, that applies
// to identifying any object in any scene.
//
// By the way, it's customary to bundle the respective distance and cell
// ID into a vector (vec3(d.x, d.y, cellID)) and return that, but I'm 
// keeping it simple here.
//
int cellTileID(in vec3 p){
    
    int cellID = 0;
    
    // Storage for the closest distance metric, second closest and the current
    // distance for comparisson testing.
    vec3 d = (vec3(.75)); // Set the maximum.
    
    // Draw some overlapping objects (spheres, in this case) at various positions on the tile.
    // Then do the fist and second order distance checks. Very simple.
    d.z = drawSphere(p - vec3(.81, .62, .53)); if(d.z<d.x) cellID = 1;
    d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z);
    
    p.xy = vec2(p.y-p.x, p.y + p.x)*.7071;
    d.z = drawSphere(p - vec3(.39, .2, .11)); if(d.z<d.x) cellID = 2;
    d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z);
    
    
    p.yz = vec2(p.z-p.y, p.z + p.y)*.7071;
    d.z = drawSphere(p - vec3(.62, .24, .06)); if(d.z<d.x) cellID = 3;
    d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z);
   
    p.xz = vec2(p.z-p.x, p.z + p.x)*.7071; 
    d.z = drawSphere(p - vec3(.2, .82, .64)); if(d.z<d.x) cellID = 4;
    d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z);

/* 
    p.xy = vec2(p.y-p.x, p.y + p.x)*.7071;
    d.z = drawSphere2(p - vec3(.48, .29, .2)); if(d.z<d.x) cellID = 5;
    d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z);
    
    p.yz = vec2(p.z-p.y, p.z + p.y)*.7071;
    d.z = drawSphere2(p - vec3(.06, .87, .78)); if(d.z<d.x) cellID = 6;
    d.y = max(d.x, min(d.y, d.z)); d.x = min(d.x, d.z); 
*/ 
    
    return cellID;
    
}


// The path is a 2D sinusoid that varies over time, depending upon the frequencies, and amplitudes.
vec2 path(in float z){
    float x = sin(z/64.)*cos(z/16.) + sin(z/16.0)*sin(z/64.0);
    float y = sin(z/32.0)*cos(z/128.) + cos(z/128.0)*cos(z/16.0);
    return (PI/2.0 - vec2(x, y)) * 9.0;
}

// Standard tunnel distance function with some perturbation thrown into the mix. A tunnel is just a tube 
// with a smoothly shifting center as you traverse lengthwise. The walls of the tube are perturbed by the
// cheap 3D surface function I described above.
float map(vec3 p){

    
    float sf = cellTile(p/2.5);
    
    // Tunnel bend correction, of sorts. Looks nice, but slays framerate, which is disappointing. I'm
    // assuming that "tan" function is the bottleneck, but I can't be sure.
    //vec2 g = (path(p.z + 0.1) - path(p.z - 0.1))/0.2;
    //g = cos(atan(g));
    p.xy -= path(p.z);
    //p.xy *= g;
  
    // Round tunnel.
    // For a round tunnel, use the Euclidean distance: length(p.xy).
    return 2.- length(p.xy*vec2(0.5, 0.7071)) + (0.5-sf)*.35;

    
/*
    // Rounded square tunnel using Minkowski distance: pow(pow(abs(tun.x), n), pow(abs(tun.y), n), 1/n)
    vec2 tun = abs(p.xy)*vec2(0.5, 0.7071);
    tun = pow(tun, vec2(8.));
    float n =1.-pow(tun.x + tun.y, 1.0/8.) + (0.5-sf)*.35;
    return n;//min(n, p.y + FH);
*/
    
/*
    // Square tunnel.
    // For a square tunnel, use the Chebyshev(?) distance: max(abs(tun.x), abs(tun.y))
    vec2 tun = abs(p.xy - path(p.z))*vec2(0.5, 0.7071);
    float n = 1.- max(tun.x, tun.y) + (0.5-sf)*.5;
    return n;
*/
 
}


// Basic raymarcher.
float trace(in vec3 ro, in vec3 rd){

    float t = 0.0, h;
    for(int i = 0; i < 96; i++){
    
        h = map(ro+rd*t);
        // Note the "t*b + a" addition. Basically, we're putting less emphasis on accuracy, as
        // "t" increases. It's a cheap trick that works in most situations... Not all, though.
        if(abs(h)<0.002*(t*.125 + 1.) || t>FAR) break; // Alternative: 0.001*max(t*.25, 1.)
        t += h*.8;
        
    }

    return min(t, FAR);
    
}

// Texture bump mapping. Four tri-planar lookups, or 12 texture lookups in total. I tried to 
// make it as concise as possible. Whether that translates to speed, or not, I couldn't say.
vec3 doBumpMap( sampler2D tx, in vec3 p, in vec3 n, float bf){
   
    const vec2 e = vec2(0.001, 0);
    
    // Three gradient vectors rolled into a matrix, constructed with offset greyscale texture values.    
    mat3 m = mat3( tex3D(tx, p - e.xyy, n), tex3D(tx, p - e.yxy, n), tex3D(tx, p - e.yyx, n));
    
    vec3 g = vec3(0.299, 0.587, 0.114)*m; // Converting to greyscale.
    g = (g - dot(tex3D(tx,  p , n), vec3(0.299, 0.587, 0.114)) )/e.x; g -= n*dot(n, g);
                      
    return normalize( n + g*bf ); // Bumped normal. "bf" - bump factor.
    
}


// Tetrahedral normal, to save a couple of "map" calls. Courtesy of IQ. By the way, there is an 
// aesthetic difference between this and the regular six tap version. Sometimes, it's noticeable,
// and other times, like this example, it's not.
vec3 calcNormal(in vec3 p){

    // Note the slightly increased sampling distance, to alleviate artifacts due to hit point inaccuracies.
    vec2 e = vec2(0.0025, -0.0025); 
    return normalize(e.xyy * map(p + e.xyy) + e.yyx * map(p + e.yyx) + e.yxy * map(p + e.yxy) + e.xxx * map(p + e.xxx));
}

/*
// Standard normal function. 6 taps.
vec3 calcNormal(in vec3 p) {
	const vec2 e = vec2(0.005, 0);
	return normalize(vec3(map(p + e.xyy) - map(p - e.xyy), map(p + e.yxy) - map(p - e.yxy),	map(p + e.yyx) - map(p - e.yyx)));
}
*/

// Ambient occlusion, for that self shadowed look. Based on the original by XT95. I love this 
// function, and in many cases, it gives really, really nice results. For a better version, and 
// usage, refer to XT95's examples below:
//
// Hemispherical SDF AO - https://www.shadertoy.com/view/4sdGWN
// Alien Cocoons - https://www.shadertoy.com/view/MsdGz2
float calculateAO( in vec3 p, in vec3 n )
{
	float ao = 0.0, l;
    const float maxDist = 2.;
	const float nbIte = 6.0;
	//const float falloff = 0.9;
    for( float i=1.; i< nbIte+.5; i++ ){
    
        l = (i*.75 + fract(cos(i)*45758.5453)*.25)/nbIte*maxDist;
        
        ao += (l - map( p + n*l ))/(1.+ l);// / pow(1.+l, falloff);
    }
	
    return clamp(1.- ao/nbIte, 0., 1.);
}

// Cool curve function, by Shadertoy user, Nimitz.
//
// From an intuitive sense, the function returns a weighted difference between a surface 
// value and some surrounding values. Almost common sense... almost. :)
//
// Original usage (I think?) - Cheap curvature: https://www.shadertoy.com/view/Xts3WM
// Other usage: Xyptonjtroz: https://www.shadertoy.com/view/4ts3z2
float curve(in vec3 p, in float w){

    vec2 e = vec2(-1., 1.)*w;
    
    float t1 = map(p + e.yxx), t2 = map(p + e.xxy);
    float t3 = map(p + e.xyx), t4 = map(p + e.yyy);
    
    return 0.125/(w*w) *(t1 + t2 + t3 + t4 - 4.*map(p));
}

/*
// Oldschool hatching effect. Interesting under the right circumstances.
vec3 ch(in vec3 col, in vec2 fragCoord){
    
    vec3 fColor = col;
    
    float lum = dot(col, vec3(.299, .587, .114));// length(col);
	float mx = 1./7.; // 1.732/7.;
    
    float rgt = fragCoord.x + fragCoord.y;
    float lft = fragCoord.x - fragCoord.y;
    
    fColor = col*4.; col *= .6;
    
    if (lum < mx*6. && mod(rgt, 8.) == 0.) fColor = col;
    if (lum < mx*5. && mod(lft, 8.) == 0.) fColor = col;
    if (lum < mx*4. && mod(rgt, 4.) == 0.) fColor = col;
    if (lum < mx*3. && mod(lft, 4.) == 0.) fColor = col;
    if (lum < mx*2. && mod(rgt, 2.) == 0.) fColor = col;
    if (lum < mx*1. && mod(lft, 2.) == 0.) fColor = col;
    
    return min(fColor, 1.);
}
*/

void mainImage( out vec4 fragColor, in vec2 fragCoord ){
	

	// Screen coordinates.
	vec2 uv = (fragCoord - iResolution.xy*0.5)/iResolution.y;
	
	// Camera Setup.
	vec3 lookAt = vec3(0.0, 0.0, iGlobalTime*6.);  // "Look At" position.
	vec3 camPos = lookAt + vec3(0.0, 0.1, -0.5); // Camera position, doubling as the ray origin.
 
    // Light positioning. One is a little behind the camera, and the other is further down the tunnel.
 	vec3 light_pos = camPos + vec3(0.0, 0.125, 4.125);// Put it a bit in front of the camera.
	vec3 light_pos2 = camPos + vec3(0.0, 0.0, 8.0);// Put it a bit in front of the camera.

	// Using the Z-value to perturb the XY-plane.
	// Sending the camera, "look at," and two light vectors down the tunnel. The "path" function is 
	// synchronized with the distance function. Change to "path2" to traverse the other tunnel.
	lookAt.xy += path(lookAt.z);
	camPos.xy += path(camPos.z);
	light_pos.xy += path(light_pos.z);
	light_pos2.xy += path(light_pos2.z);

    // Using the above to produce the unit ray-direction vector.
    float FOV = PI/3.; // FOV - Field of view.
    vec3 forward = normalize(lookAt-camPos);
    vec3 right = normalize(vec3(forward.z, 0., -forward.x )); 
    vec3 up = cross(forward, right);

    // rd - Ray direction.
    vec3 rd = normalize(forward + FOV*uv.x*right + FOV*uv.y*up);
    
    // Swiveling the camera from left to right when turning corners.
    //rd.x = rot2( path(lookAt.z).x/32. )*rd.x;
		
    // Standard ray marching routine.
    float t = trace(camPos, rd);
	
    // The final scene color. Initated to black.
	vec3 sceneCol = vec3(0.);
	
	// The ray has effectively hit the surface, so light it up.
	if(t < FAR){
	
    	
    	// Surface position and surface normal.
	    vec3 sp = t * rd+camPos;
	    vec3 sn = calcNormal(sp);
        
        // Texture scale factor.
        const float tSize0 = 1./1.; 
        const float tSize1 = 1./1.;
    	
    	// Texture-based bump mapping.
	    //if (sp.y<-(FH-0.005)) sn = doBumpMap(iChannel1, sp*tSize1, sn, 0.025); // Floor.
	    //else sn = doBumpMap(iChannel0, sp*tSize0, sn, 0.025); // Walls.
        
        //sn = doBumpMap(iChannel1, sp*tSize0, sn, 0.02);
        //sn = doBumpMap(sp, sn, 0.01);
	    
	    // Ambient occlusion.
	    float ao = calculateAO(sp, sn);
    	
    	// Light direction vectors.
	    vec3 ld = light_pos-sp;
	    vec3 ld2 = light_pos2-sp;

        // Distance from respective lights to the surface point.
	    float lDdist = max(length(ld), 0.001);
	    float lDdist2 = max(length(ld2), 0.001);
    	
    	// Normalize the light direction vectors.
	    ld /= lDdist;
	    ld2 /= lDdist2;
	    
	    // Light attenuation, based on the distances above. In case it isn't obvious, this
        // is a cheap fudge to save a few extra lines. Normally, the individual light
        // attenuations would be handled separately... No one will notice, or care. :)
	    float atten = 1./(1. + lDdist*.125 + lDdist*lDdist*.05);
        float atten2 =  1./(1. + lDdist2*.125 + lDdist2*lDdist2*.05);
    	
    	// Ambient light.
	    float ambience = 0.75;
    	
    	// Diffuse lighting.
	    float diff = max( dot(sn, ld), 0.0);
	    float diff2 = max( dot(sn, ld2), 0.0);
    	
    	// Specular lighting.
	    float spec = pow(max( dot( reflect(-ld, sn), -rd ), 0.0 ), 64.);
	    float spec2 = pow(max( dot( reflect(-ld2, sn), -rd ), 0.0 ), 64.);
    	
    	// Curvature.
	    float crv = clamp(curve(sp, 0.125)*0.5+0.5, .0, 1.);
	    
	    // Fresnel term. Good for giving a surface a bit of a reflective glow.
        float fre = pow( clamp(dot(sn, rd) + 1., .0, 1.), 1.);
 
        
        vec3 texCol = vec3(0.08,0,0.0);//tex3D(iChannel0, sp*tSize0, sn);
        
        //texCol = min(texCol*1.5, 1.);
        //texCol = vec3(1)*dot(texCol, vec3(0.299, 0.587, 0.114));
        //texCol = smoothstep(-.0, .6, texCol); // etc.
        
        //texCol = texCol*vec3(1., .5, .2); 
        int id = cellTileID(sp/2.5);
        if(id == 4) texCol = texCol*vec3(1., 0.5, .3); 
        if(id == 3) texCol = texCol*.5 + texCol*vec3(.5, .25, .15); 
        
    	
    	// Darkening the crevices. Otherwise known as cheap, scientifically-incorrect shadowing.	
	    float shading =  crv*0.75+0.25; 
    	
        //Dain: put some green in the cracks
        vec3 crevColor = vec3(0.6,0.6,0.6);//tex3D(iChannel2, sp*tSize0, sn);
        texCol = mix(texCol, crevColor, pow(1.0 - crv, 16.0));
       // ambience *= crv;
    	// Combining the above terms to produce the final color. It was based more on acheiving a
        // certain aesthetic than science.
        //
        // Shiny.
        sceneCol = (texCol*(diff + ambience + spec) + spec*vec3(.7, .9, 1))*atten;
        sceneCol += (texCol*(diff2 + ambience + spec2) + spec2*vec3(.7, .9, 1))*atten2;
        //
        // Other combinations:
        //
        // Glow.
        //float gr = dot(texCol, vec3(0.299, 0.587, 0.114));
        //sceneCol = (gr*(diff + ambience*0.25) + spec*texCol*2. + fre*crv*texCol.zyx*2.)*atten;
        //sceneCol += (gr*(diff2 + ambience*0.25) + spec2*texCol*2. + fre*crv*texCol.zyx*2.)*atten2;
        
        // Shading.
        sceneCol *= shading*ao;
        
        //Dain: turned off the lines to show the smooth surface
        // Drawing the lines on the surface.      
       // sceneCol *= clamp(abs(curve(sp, 0.035)), .0, 1.)*.5 + 1.;  // Glow lines.
       // sceneCol *= 1. - smoothstep(0., 4., abs(curve(sp, 0.0125)))*vec3(.82, .85, .88); // Darker.
	   
	
	}
    
    // Some simple post processing effects.
    //float a = dot(sceneCol, vec3(0.299, 0.587, 0.114));
    //sceneCol = min(vec3(a*3., pow(a, 2.5)*2., pow(a, 6.)), 1.); // Fire palette.
    //sceneCol = floor(sceneCol*15.999)/15.; // Oldschool effect.  
    // Oldschool hatching effect. Uncomment the "ch" function to use this one.
    //sceneCol = ch(clamp(sceneCol, 0., 1.), fragCoord); 
	
	fragColor = vec4(sqrt(clamp(sceneCol, 0., 1.)), 1.0);
    
	
}

void main( void ) {
mainImage(gl_FragColor,gl_FragCoord.xy);

}

)";

const char* explosion = R"(

#ifdef GL_ES
precision highp float;
#endif

#extension GL_OES_standard_derivatives : enable

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

// port from https://www.shadertoy.com/view/lsySzd
// "Volumetric explosion" by Duke
//-------------------------------------------------------------------------------------
// Based on "Supernova remnant" (https://www.shadertoy.com/view/MdKXzc) 
// and other previous shaders 
// otaviogood's "Alien Beacon" (https://www.shadertoy.com/view/ld2SzK)
// and Shane's "Cheap Cloud Flythrough" (https://www.shadertoy.com/view/Xsc3R4) shaders
// Some ideas came from other shaders from this wonderful site
// Press 1-2-3 to zoom in and zoom out.
// License: Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License
//-------------------------------------------------------------------------------------

// Whoever forced "BOTH" to be the default was a complete jackass...way to ruin a beautiful shader...
//
// comment this string to see each part in full screen
//#define BOTH
// uncomment this string to see left part

#define LEFT
				// B U R N  !
#define DITHERING

//#define TONEMAPPING

//-------------------
#define pi 3.14159265
#define R(p, a) p=cos(a)*p+sin(a)*vec2(p.y, -p.x)

vec3 hash( vec3 p )
{
	p = vec3( dot(p,vec3(127.1,311.7, 74.7)),
			  dot(p,vec3(269.5,183.3,246.1)),
			  dot(p,vec3(113.5,271.9,124.6)));

	return -1.0 + 2.0*fract(sin(p)*43758.5453123);
}

float noise( in vec3 p )
{
    vec3 i = floor( p );
    vec3 f = fract( p );
	
	vec3 u = f*f*(3.0-2.0*f);

    return mix( mix( mix( dot( hash( i + vec3(0.0,0.0,0.0) ), f - vec3(0.0,0.0,0.0) ), 
                          dot( hash( i + vec3(1.0,0.0,0.0) ), f - vec3(1.0,0.0,0.0) ), u.x),
                     mix( dot( hash( i + vec3(0.0,1.0,0.0) ), f - vec3(0.0,1.0,0.0) ), 
                          dot( hash( i + vec3(1.0,1.0,0.0) ), f - vec3(1.0,1.0,0.0) ), u.x), u.y),
                mix( mix( dot( hash( i + vec3(0.0,0.0,1.0) ), f - vec3(0.0,0.0,1.0) ), 
                          dot( hash( i + vec3(1.0,0.0,1.0) ), f - vec3(1.0,0.0,1.0) ), u.x),
                     mix( dot( hash( i + vec3(0.0,1.0,1.0) ), f - vec3(0.0,1.0,1.0) ), 
                          dot( hash( i + vec3(1.0,1.0,1.0) ), f - vec3(1.0,1.0,1.0) ), u.x), u.y), u.z );
}
float fbm( vec3 p )
{
   return noise(p*.06125)*.5 + noise(p*.125)*.25 + noise(p*.25)*.125 + noise(p*.4)*.2;
}

float rand(vec2 co)
{
   return fract(sin(dot(co*0.123,vec2(12.9898,78.233))) * 43758.5453);
}

float Sphere( vec3 p, float r )
{
   return length(p)-r;
}

//==============================================================
// otaviogood's noise from https://www.shadertoy.com/view/ld2SzK
//--------------------------------------------------------------
// This spiral noise works by successively adding and rotating sin waves while increasing frequency.
// It should work the same on all computers since it's not based on a hash function like some other noises.
// It can be much faster than other noise functions if you're ok with some repetition.
const float nudge = 4.;	// size of perpendicular vector
float normalizer = 1.0 / sqrt(1.0 + nudge*nudge);	// pythagorean theorem on that perpendicular to maintain scale
float SpiralNoiseC(vec3 p)
{
    float n = -mod(time * 0.2,-2.); // noise amount
    float iter = 2.0;
    for (int i = 0; i < 8; i++)
    {
        // add sin and cos scaled inverse with the frequency
        n += -abs(sin(p.y*iter) + cos(p.x*iter)) / iter;	// abs for a ridged look
        // rotate by adding perpendicular and scaling down
        p.xy += vec2(p.y, -p.x) * nudge;
        p.xy *= normalizer;
        // rotate on other axis
        p.xz += vec2(p.z, -p.x) * nudge;
        p.xz *= normalizer;
        // increase the frequency
        iter *= 1.733733;
    }
    return n;
}

float VolumetricExplosion(vec3 p)
{
    float final = Sphere(p,4.);
    #ifdef LOW_QUALITY
    final += noise(p*12.5)*.2;
    #else
    final += fbm(p*50.);
    #endif
    final += SpiralNoiseC(p.zxy*0.4132+333.)*3.0; //1.25;

    return final;
}

float map(vec3 p) 
{
	R(p.xz, mouse.x*0.008*pi+time*0.1);

	float VolExplosion = VolumetricExplosion(p/0.5)*0.5; // scale
    
	return VolExplosion;
}
//--------------------------------------------------------------

// assign color to the media
vec3 computeColor( float density, float radius )
{
	// color based on density alone, gives impression of occlusion within
	// the media
	vec3 result = mix( vec3(1.0,0.9,0.8), vec3(0.4,0.15,0.1), density );
	
	// color added to the media
	vec3 colCenter = 7.*vec3(0.8,1.0,1.0);
	vec3 colEdge = 1.5*vec3(0.48,0.53,0.5);
	result *= mix( colCenter, colEdge, min( (radius+.05)/.9, 1.15 ) );
	
	return result;
}

bool RaySphereIntersect(vec3 org, vec3 dir, out float near, out float far)
{
	float b = dot(dir, org);
	float c = dot(org, org) - 8.;
	float delta = b*b - c;
	if( delta < 0.0) 
		return false;
	float deltasqrt = sqrt(delta);
	near = -b - deltasqrt;
	far = -b + deltasqrt;
	return far > 0.0;
}

// Applies the filmic curve from John Hable's presentation
// More details at : http://filmicgames.com/archives/75
vec3 ToneMapFilmicALU(vec3 _color)
{
	_color = max(vec3(0), _color - vec3(0.004));
	_color = (_color * (6.2*_color + vec3(0.5))) / (_color * (6.2 * _color + vec3(1.7)) + vec3(0.06));
	return _color;
}

void main( void )
{  

    	vec2 uv = gl_FragCoord.xy/resolution.xy;
    
	// ro: ray origin
	// rd: direction of the ray
	vec3 rd = normalize(vec3((gl_FragCoord.xy-0.5*resolution.xy)/resolution.y, 1.0));
	vec3 ro = vec3(0., 0., -6.);
    	
	#ifdef DITHERING
	vec2 seed = uv + fract(time);
	#endif 
	
	// ld, td: local, total density 
	// w: weighting factor
	float ld=0., td=0., w=0.;

	// t: length of the ray
	// d: distance function
	float d=1., t=0.;
    
    	const float h = 0.1;
   
	vec4 sum = vec4(0.0);
   
    	float min_dist=0.0, max_dist=0.0;

   	if(RaySphereIntersect(ro, rd, min_dist, max_dist))
    	{
       
	t = min_dist*step(t,min_dist);
   
	// raymarch loop
    	#ifdef LOW_QUALITY
	for (int i=0; i<6; i++)
    	#else
    	for (int i=0; i<86; i++)
    	#endif
	{
	 
	    vec3 pos = ro + t*rd;
  
	    // Loop break conditions.
	    if(td>0.9 || d<0.12*t || t>10. || sum.a > 0.99 || t>max_dist) break;
        
	    // evaluate distance function
	    float d = map(pos);
        
	    #ifdef BOTH
	    if (uv.x<1.0)
	    {
            	d = abs(d)+0.07;
	    }
	    #else
	    #ifdef LEFT
	    d = abs(d)+0.07;
	    #endif
	    #endif
        
	    // change this string to control density 
	    d = max(d,0.03);
        
	    // point light calculations
	    vec3 ldst = vec3(0.0)-pos;
	    float lDist = max(length(ldst), 0.001);
	    
	    // the color of light 
	    vec3 lightColor=vec3(1.0,0.5,0.25);
        
	    sum.rgb+=(lightColor/exp(lDist*lDist*lDist*.08)/30.); // bloom
        
	    if (d<h) 
	    {
	    // compute local density 
	    ld = h - d;
            
	    // compute weighting factor 
	    w = (1. - td) * ld;
     
	    // accumulate density
	    td += w + 1./200.;
		
	    vec4 col = vec4( computeColor(td,lDist), td );
            
            // emission
            sum += sum.a * vec4(sum.rgb, 0.0) * 0.2 / lDist;	
            
	    // uniform scale density
	    col.a *= 0.2;
	    // colour by alpha
	    col.rgb *= col.a;
	    // alpha blend in contribution
	    sum = sum + col*(1.0 - sum.a);  
       
	    }
      
	    td += 1./70.;
		
	    #ifdef DITHERING
	    d=abs(d)*(.8+0.2*rand(seed*vec2(i)));
	    #endif 
		
	    // trying to optimize step size
	    #ifdef LOW_QUALITY
	    t += max(d*0.25,0.01);
	    #else
	    t += max(d * 0.08 * max(min(length(ldst),d),2.0), 0.01);
	    #endif
                
	}
    
	// simple scattering
	#ifdef LOW_QUALITY    
	sum *= 1. / exp( ld * 0.2 ) * 0.9;
	#else
	sum *= 1. / exp( ld * 0.2 ) * 0.8;
	#endif
        
   	sum = clamp( sum, 0.0, 1.0 );
   
	sum.xyz = sum.xyz*sum.xyz*(3.0-2.0*sum.xyz);
    
	}
   
	#ifdef TONEMAPPING
	gl_FragColor = vec4(ToneMapFilmicALU(sum.xyz*2.2),1.0);
	#else
	gl_FragColor = vec4(sum.xyz,1.0);
	#endif
}

)";

const char* rainbow = R"(

#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 resolution;

#define PI 3.14159265359
#define T (time/2.)

vec3 hsv2rgb(vec3 c)
{
    vec4 K = vec4(1.0, 2.0 / 3.0, 1.0 / 3.0, 3.0);
    vec3 p = abs(fract(c.xxx + K.xyz) * 6.0 - K.www);
    return c.z * mix(K.xxx, clamp(p - K.xxx, 0.0, 1.0), c.y);
}

void main( void ) {

	vec2 position = (( gl_FragCoord.xy / resolution.xy ) - 0.5);
	position.x *= resolution.x / resolution.y;
	
	vec3 color = vec3(0.);
	
	for (float i = 0.; i < PI*2.; i += PI/17.5) {
		vec2 p = position - vec2(cos(i+T), sin(i+T)) * 0.25;
		vec3 col = hsv2rgb(vec3((i)/(PI*2.), 1., mod(i-T*3.,PI*2.)/PI));
		color += col * (1./128.)
			/ length(p);
	}

	gl_FragColor = vec4( color, 1.0 );

}

)";

const char* hexagons = R"(

#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

vec2 rotate(vec2 p, float a)
	{
	return vec2(p.x * cos(a) - p.y * sin(a), p.x * sin(a) + p.y * cos(a));
	}
float box(vec2 p, vec2 b, float r)
	{
	return length(max(abs(p) - b, 0.0)) - r;
	}

vec3 intersect(in vec3 o, in vec3 d, vec3 c, vec3 u, vec3 v)
	{
	vec3 q = o - c;
	return vec3(
	dot(cross(u, v), q),
	dot(cross(q, u), d),
	dot(cross(v, q), d)) / dot(cross(v, u), d);
	}

float rand11(float p)
	{
	return fract(sin(p * 591.32) * 43758.5357);
	}
float rand12(vec2 p)
	{
	return fract(sin(dot(p.xy, vec2(12.9898, 78.233))) * 43758.5357);
	}
vec2 rand21(float p)
	{
	return fract(vec2(sin(p * 591.32), cos(p * 391.32)));
	}

vec2 rand22(in vec2 p)
	{
	return fract(vec2(sin(p.x * 591.32 + p.y * 154.077), cos(p.x * 391.32 + p.y * 49.077)));
	}

float noise11(float p)
	{
	float fl = floor(p);
	return mix(rand11(fl), rand11(fl + 1.0), fract(p));//smoothstep(0.0, 1.0, fract(p)));
	}
float fbm11(float p)
	{
	return noise11(p) * 0.5 + noise11(p * 2.0) * 0.25 + noise11(p * 5.0) * 0.125;
	}
vec3 noise31(float p)
	{
	return vec3(noise11(p), noise11(p + 18.952), noise11(p - 11.372)) * 2.0 - 1.0;
	}

float sky(vec3 p)
	{
	float a = atan(p.x, p.z);
	float t = time * 0.1;
	float v = rand11(floor(a * 4.0 + t)) * 0.5 + rand11(floor(a * 8.0 - t)) * 0.25 + rand11(floor(a * 16.0 + t)) * 0.125;
	return v;
	}

vec3 voronoi(in vec2 x)
	{
	vec2 n = floor(x); // grid cell id
	vec2 f = fract(x); // grid internal position
	vec2 mg; // shortest distance...
	vec2 mr; // ..and second shortest distance
	float md = 8.0, md2 = 8.0;
	for(int j = -1; j <= 1; j ++)
		{
		for(int i = -1; i <= 1; i ++)
			{
			vec2 g = vec2(float(i), float(j)); // cell id
			vec2 o = rand22(n + g); // offset to edge point
			vec2 r = g + o - f;
			
			float d = max(abs(r.x), abs(r.y)); // distance to the edge
			
			if(d < md)
				{
				md2 = md; md = d; mr = r; mg = g;
				}
			else if(d < md2)
				{
				md2 = d;
				}
			}
		}
	return vec3(n + mg, md2 - md);
	}

#define A2V(a) vec2(sin((a) * 6.28318531 / 100.0), cos((a) * 6.28318531 / 100.0))

float circles(vec2 p)
	{
	float v, w, l, c;
	vec2 pp;
	l = length(p);
	
	
	pp = rotate(p, time * 3.0);
	c = max(dot(pp, normalize(vec2(-0.2, 0.5))), -dot(pp, normalize(vec2(0.2, 0.5))));
	c = min(c, max(dot(pp, normalize(vec2(0.5, -0.5))), -dot(pp, normalize(vec2(0.2, -0.5)))));
	c = min(c, max(dot(pp, normalize(vec2(0.3, 0.5))), -dot(pp, normalize(vec2(0.2, 0.5)))));
	
	// innerest stuff
	v = abs(l - 0.5) - 0.03;
	v = max(v, -c);
	v = min(v, abs(l - 0.54) - 0.02);
	v = min(v, abs(l - 0.64) - 0.05);
	
	pp = rotate(p, time * -1.333);
	c = max(dot(pp, A2V(-5.0)), -dot(pp, A2V(5.0)));
	c = min(c, max(dot(pp, A2V(25.0 - 5.0)), -dot(pp, A2V(25.0 + 5.0))));
	c = min(c, max(dot(pp, A2V(50.0 - 5.0)), -dot(pp, A2V(50.0 + 5.0))));
	c = min(c, max(dot(pp, A2V(75.0 - 5.0)), -dot(pp, A2V(75.0 + 5.0))));
	
	w = abs(l - 0.83) - 0.09;
	v = min(v, max(w, c));
	
	return v;
	}

float shade1(float d)
	{
	float v = 1.0 - smoothstep(0.0, mix(0.012, 0.2, 0.0), d);
	float g = exp(d * -20.0);
	return v + g * 0.5;
	}


void main()
	{
	vec2 uv = gl_FragCoord.xy / resolution.xy;
	uv = uv * 2.0 - 1.0;
	uv.x *= resolution.x / resolution.y;
	
	
	// using an iq styled camera this time :)
	// ray origin
	vec3 ro = 0.7 * vec3(cos(0.2 * time/4.0), 0.0, sin(0.2 * time/4.0));
	ro.y = cos(0.6 * time/4.0) * 0.1 + 0.15;
	// camera look at
	vec3 ta = vec3(0.0, 0.0, 0.0);
	
	// camera shake intensity
	float shake = clamp(0.0 * (1.0 - length(ro.yz)), 0.3, 1.0);
	float st = mod(time, 10.0) * 043.0;
	
	// build camera matrix
	vec3 ww = normalize(ta - ro + noise31(st) * shake * 0.01);
	vec3 uu = normalize(cross(ww, normalize(vec3(0.0, 1.0, 0.1 * sin(time/2.0)))));
	vec3 vv = normalize(cross(uu, ww));
	// obtain ray direction
	vec3 rd = normalize(uv.x * uu + uv.y * vv + 1.0 * ww);
	
	// shaking and movement
	//ro += noise31(-st) * shake * 0.005;
	ro.x += time * 1.0;
	
	float inten = 0.0;
	
	// background
	float sd = dot(rd, vec3(0.0, 1.0, 0.0));
	inten = pow(1.0 - abs(sd), 20.0) + pow(sky(rd), 5.0) * step(0.0, rd.y) * 0.2;
	
	vec3 its;
	float v, g;
	
	// voronoi floor layers
	for(int i = 0; i < 4; i ++)
		{
		float layer = float(i);
		its = intersect(ro, rd, vec3(0.0, -5.0 - layer * 5.0, 0.0), vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0));
		if(its.x > 0.0)
			{
			vec3 vo = voronoi((its.yz) * 0.05 + 8.0 * rand21(float(i)));
			v = exp(-100.0 * (vo.z - 0.02));
			
			float fx = 0.0;
			
			// add some special fx to lowest layer
			if(i == 3)
				{
				float crd = 0.0;//fract(time * 0.2) * 50.0 - 25.0;
				float fxi = cos(vo.x * 0.2 + time * 1.5);//abs(crd - vo.x);
				fx = clamp(smoothstep(0.9, 1.0, fxi), 0.0, 0.9) * 1.0 * rand12(vo.xy);
				fx *= exp(-3.0 * vo.z) * 2.0;
				}
			inten += v * 0.1 + fx;
			}
		}
	
	// draw the gates, 4 should be enough
	float gatex = floor(ro.x / 8.0 + 0.5) * 8.0 + 4.0;
	float go = -16.0;
	for(int i = 0; i < 0; i ++)
		{
		its = intersect(ro, rd, vec3(gatex + go, 0.0, 0.0), vec3(0.0, 1.0, 0.0), vec3(0.0, 0.0, 1.0));
		if(dot(its.yz, its.yz) < 2.0 && its.x > 0.0)
			{
			v = circles(its.yz);
			inten += shade1(v);
			}
		
		go += 8.0;
		}
	
	// draw the stream
	for(int j = 0; j < 0; j ++)
		{
		float id = float(j);
		
		vec3 bp = vec3(0.0, (rand11(id) * 2.0 - 1.0) * 0.25, 0.0);
		vec3 its = intersect(ro, rd, bp, vec3(1.0, 0.0, 0.0), vec3(0.0, 0.0, 1.0));
		
		if(its.x > 0.0)
			{
			vec2 pp = its.yz;
			float spd = (2.0 + rand11(id) * 3.0) * 2.5;
			pp.y += time * spd;
			pp += (rand21(id) * 2.0 - 1.0) * vec2(0.3, 1.0);
			float rep = rand11(id) + 1.5;
			pp.y = mod(pp.y, rep * 2.0) - rep;
			float d = box(pp, vec2(0.02, 0.3), 0.1);
			float foc = 0.0;
			float v = 1.0 - smoothstep(0.0, 0.03, abs(d) - 0.001);
			float g = min(exp(d * -20.0), 2.0);
			
			inten += (v + g * 0.7) * 0.5;
			
			}
		}
	
	inten *= 0.4 + (sin(time) * 0.5 + 0.5) * 0.6;
	
	// find a color for the computed intensity
	#ifdef GREEN_VERSION
	vec3 col = pow(vec3(inten), vec3(1.0, 0.15, 9.0));
	#else
	vec3 col = pow(vec3(inten), 1.5 * vec3(3.15, 2.0, 1.0));
	#endif
	//col.z = col.x;
	//col.x = 0.0;		
	gl_FragColor = vec4(col, 1.0);
	}

)";

const char* fractal = R"(

// have a nice friday Kai!

#ifdef GL_ES
precision mediump float;
#endif

#extension GL_OES_standard_derivatives : enable

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

vec4 orb = vec4(1000);
float de( vec3 p ,float s)
{	
	float scale = 1.0;

	orb = vec4(1000.0); 
	
	for( int i=0; i<8;i++ )
	{
		p = -1.0 + 2.0*fract(0.5*p+0.5);

		float r2 = dot(p,p);
		
      			  orb = min( orb, vec4(abs(p),r2) );
		
		float k = s/r2;
		p     *= k;
		scale *= k;
	}
	
	return 0.25*abs(p.y)/scale;

}


vec3 normal( in vec3 pos, in float t, in float s )
{
    float pre = 0.001 * t;
    vec2 e = vec2(1.0,-1.0)*pre;
    return normalize( e.xyy*de( pos + e.xyy, s ) + 
					  e.yyx*de( pos + e.yyx, s ) + 
					  e.yxy*de( pos + e.yxy, s ) + 
                      e.xxx*de( pos + e.xxx, s ) );
}


float raymarch(in vec3 from, in vec3 dir,float s) {
	
    float maxd = 30.0;
    float t = 0.01;
    for( int i=0; i<250; i++ )
    {
	    float precis = 0.001 * t;
        
	   float h = de( from+dir*t, s );
        if( h<precis||t>maxd ) break;
        t += h;
    }

    if( t>maxd ) t=-1.0;
    return t;
}

float tri(in float x){return abs(fract(x)-.5);}
vec3 tri3(in vec3 p){return vec3( tri(p.z+tri(p.y*1.)), tri(p.z+tri(p.x*1.)), tri(p.y+tri(p.x*1.)));}

float triNoise3d(in vec3 p, in float spd)
{
    float z=1.4;
	float rz = 0.;
    vec3 bp = p;
	for (float i=0.; i<=3.; i++ )
	{
        vec3 dg = tri3(bp*2.);
        p += (dg+time*spd);

        bp *= 1.8;
		z *= 1.5;
		p *= 1.2;
        
        rz+= (tri(p.z+tri(p.x+tri(p.y))))/z;
        bp += 0.14;
	}
	return rz;
}

float fogmap(in vec3 p, in float d)
{
    p.x += time*2.5;
    p.z += sin(p.x*.5);
    return triNoise3d(p*2.2/(d+20.),0.2)*(1.-smoothstep(0.,.7,p.y));
}

vec3 fog(in vec3 col, in vec3 ro, in vec3 rd, in float mt)
{
    float d = .5;
    for(int i=0; i<7; i++)
    {
        vec3  pos = ro + rd*d;
        float rz = fogmap(pos, d);
		float grd =  clamp((rz - fogmap(pos+.8-float(i)*0.1,d))*3., 0.1, 1. );
	    
        vec3 col2 = (vec3(.8,0.8,.5)*.6 + .5*vec3(.5, .8, 1.)*(1.7-grd))*1.55;
	    
        col = mix(col,col2,clamp(rz*smoothstep(d-0.4,d+2.+d*.75,mt),0.,1.) );
        d *= 1.5+0.3;
        if (d>mt)break;
    }
    return col;
}



vec3 postProcess(vec3 color){
	color*=vec3(1.,.94,.87);
	color=pow(color,vec3(1.3));
	color=mix(vec3(length(color)),color,.85)*0.95;
	return color;
}


void main( void ) {

	 float tm = time*0.35 ;
	
	vec2 uv = (gl_FragCoord.xy / resolution) - .5;
	
	  float anim = 1.1 + 0.5*smoothstep( -0.3, 0.3, cos(0.1*time) );
	
	
	 vec3 ro = vec3( 2.8*cos(0.1+.33*tm), 0.4 + 0.30*cos(0.37*tm), 2.8*cos(0.5+0.35*tm) );
        vec3 ta = vec3( 1.9*cos(1.2+.41*tm), 0.4 + 0.10*cos(0.27*time), 1.9*cos(2.0+0.38*tm) );
        float roll =  0.2*cos(0.1*time);
        vec3 cw = normalize(ta-ro);
        vec3 cp = vec3(sin(roll), cos(roll),0.0);
        vec3 cu = normalize(cross(cw,cp));
        vec3 cv = normalize(cross(cu,cw));
        vec3 rd = normalize( uv.x*cu + uv.y*cv + 2.0*cw );

	float t = raymarch(ro,rd,anim);
	
	vec3 col = vec3(0.0);
	col+=vec3(1,.85,.7)*pow(max(0.,.3-length(uv-vec2(0.,.03)))/.3,1.5)*.35;
	
	vec3  light1 = vec3(  0.577, 0.577, -0.577 );
        vec3  light2 = vec3( -0.707, 0.000,  0.707 );
	
	if(t > 0.0){
	vec4 tra = orb;
        vec3 pos = ro + t*rd;
        vec3 nor = normal( pos, t, anim );

        
        float key = clamp( dot( light1, nor ), 0.0, 1.0 );
        float bac = clamp( 0.2 + 0.8*dot( light2, nor ), 0.0, 1.0 );
        float amb = (0.7+0.3*nor.y);
        float ao = pow( clamp(tra.w*2.0,0.0,1.0), 1.2 );
		
	vec3 ref = reflect (pos, nor);
		

        vec3 brdf  = 1.0*vec3(0.40,0.40,0.40)*amb*ao;
        brdf += 1.0*vec3(1.00,1.00,1.00)*key*ao;
        brdf += 1.0*vec3(0.40,0.40,0.40)*bac*ao;

         vec3 rgb = vec3(1.0);
       	 rgb = mix( rgb, vec3(1.0,0.80,0.2), clamp(6.0*tra.y,0.0,1.0) );
       	 rgb = mix( rgb, vec3(1.0,0.55,0.0), pow(clamp(1.0-2.0*tra.z,0.0,1.0),8.0) );

     	 col = rgb*brdf*exp(-0.0*t);
		
		
		
		col = sqrt(	col);
    	}
	else{
		col = vec3(0);
		
			
	}
	
	col = fog(col,rd, ro, 3.5);
	
	
	

	gl_FragColor = vec4( postProcess(col),1.);

}

)";

const char* cubes = R"(

// Hauva Kukka 2014.
#ifdef GL_ES
precision mediump float;
#endif

#extension GL_OES_standard_derivatives : enable

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;



#define DISTANCE_EPSILON 0.01
#define NORMAL_DELTA 0.0001
#define OCCLUSION_DELTA 0.1
#define SCENE_RADIUS 500.0
#define STEP_SCALE 1.0

#define MAX_HIT_COUNT 5
#define ITERATION_COUNT 100

#define TYPE_NONE        0
#define TYPE_OPAQUE      1
#define TYPE_TRANSPARENT 2

#define MODE_OUTSIDE 0
#define MODE_INSIDE  1

#define N1 1.0
#define N2 2.0
 
#define R0(x, y) (((x - y) / (x + y)) * ((x - y) / (x + y)))
#define PI 3.1415

#define SHADOW
// lazy gigatron ;
#define fragCoord gl_FragCoord.xy
#define iTime time
#define iResolution resolution
#define fragColor gl_FragColor
// End Shadertoy

struct Material {
	vec3 color;
    float shininess;
    float opacity;
};

struct HitInfo {
	Material material;
    int type;
};
        
vec3 viewer;
vec3 target;

HitInfo hitInfo;

float clock;

vec3 mirrorRay(vec3 ray, vec3 normal) {
	float dot = dot(ray, normal);
    return 2.0 * dot * normal - ray;
}

float getPatternWeight(vec3 position) {
    if (mod(position.z, 1.788854) > 0.894427) position.x += 0.5;

    vec2 local = vec2(mod(position.x, 1.000000), mod(position.z, 0.894427));
    vec2 center = vec2(0.5, 0.447214);    

    vec2 delta = center - local;                  
	if (dot(delta, delta) > 0.18) return 0.0;
    else                          return 1.0;    
}

float getDistanceBox(vec3 position, vec3 boxRadius, float r) {
	vec3 delta = abs(position) - boxRadius;
	return min(max(delta.x, max(delta.y, delta.z)), 0.0) + length(max(delta, 0.0)) - r;
}

float getDistancePlaneXZ(vec3 position, float height) {
	return position.y - height;
}

mat2 getRotation(float r) {
 	return mat2(cos(r), sin(r), -sin(r), cos(r));
}

float getDistanceSceneOpaque(vec3 position, bool saveHit) {
    float field = getDistancePlaneXZ(position, -0.5);  
    
    if (field < DISTANCE_EPSILON && saveHit) {
		hitInfo.type = TYPE_OPAQUE;
        hitInfo.material.color = mix(
            vec3(0.9, 0.8, 0.8),
            vec3(0.9, 0.2, 0.2),
    		getPatternWeight(position)
        );
        
        hitInfo.material.shininess = 0.1;
        hitInfo.material.opacity = 1.0;
   	}	

	return field;
}

float getDistanceSceneTransparent(vec3 position, bool saveHit) {   
    mat2 twirl = mat2(cos(-clock), sin(-clock), -sin(-clock), cos(-clock));
    vec3 local = position;
    
    local.xyz = position.xyz - vec3(target.x, sin(mod(clock, 0.5 * PI) + 0.25 * PI), 0.0);
    local.xy = getRotation(-clock) * local.xy;
    float field = getDistanceBox(local, vec3(1.0), 0.2);	

    local = position;
    local.x -= 1.0 * target.x + 2.5;
    local.y -= 0.5 *  sin(mod(2.0 * clock, 0.5 * PI) + 0.25 * PI);
    local.xy = twirl * twirl * local.xy;
    local.z -= 2.2;

    field = min(field, getDistanceBox(local, vec3(0.5), 0.1));
    
    local = position;
    local.x -= 1.0 * target.x + 0.5;
    local.y -= 0.5 *  sin(mod(2.0 * clock, 0.5 * PI) + 0.25 * PI);
    local.xy = twirl * twirl * local.xy;
    local.z -= 2.2;
    
    field = min(field, getDistanceBox(local, vec3(0.5), 0.1));

    if (field < DISTANCE_EPSILON && saveHit) {
        hitInfo.type = TYPE_TRANSPARENT;
        hitInfo.material.color = vec3(1.0, 0.9, 0.8);
        hitInfo.material.shininess = 7.0;
        hitInfo.material.opacity = 0.025;
    }

    return field;
}

float getDistanceScene(vec3 position, bool saveHit) {
	return min(
        getDistanceSceneOpaque(position, saveHit),
        getDistanceSceneTransparent(position, saveHit)
    );    
}
   
vec3 getNormalTransparent(vec3 position) {
    vec3 nDelta = vec3(
        getDistanceSceneTransparent(position - vec3(NORMAL_DELTA, 0.0, 0.0), false),
        getDistanceSceneTransparent(position - vec3(0.0, NORMAL_DELTA, 0.0), false),
        getDistanceSceneTransparent(position - vec3(0.0, 0.0, NORMAL_DELTA), false)
   	);
    
    vec3 pDelta = vec3(
        getDistanceSceneTransparent(position + vec3(NORMAL_DELTA, 0.0, 0.0), false),
        getDistanceSceneTransparent(position + vec3(0.0, NORMAL_DELTA, 0.0), false),
        getDistanceSceneTransparent(position + vec3(0.0, 0.0, NORMAL_DELTA), false)
   	);
     
    return normalize(pDelta - nDelta);
}

vec3 getNormalOpaque(vec3 position) {
    vec3 nDelta = vec3(
        getDistanceSceneOpaque(position - vec3(NORMAL_DELTA, 0.0, 0.0), false),
        getDistanceSceneOpaque(position - vec3(0.0, NORMAL_DELTA, 0.0), false),
        getDistanceSceneOpaque(position - vec3(0.0, 0.0, NORMAL_DELTA), false)
   	);
    
    vec3 pDelta = vec3(
        getDistanceSceneOpaque(position + vec3(NORMAL_DELTA, 0.0, 0.0), false),
        getDistanceSceneOpaque(position + vec3(0.0, NORMAL_DELTA, 0.0), false),
        getDistanceSceneOpaque(position + vec3(0.0, 0.0, NORMAL_DELTA), false)
   	);
     
    return normalize(pDelta - nDelta);
}

float getSoftShadow(vec3 position, vec3 normal) {
    position += DISTANCE_EPSILON * normal;
    
    float delta = 1.0;
    float minimum = 1.0;
    for (int i = 0; i < ITERATION_COUNT; i++) {
        float field = max(0.0, getDistanceSceneTransparent(position, false));
        if (field < DISTANCE_EPSILON) return 0.3;

        minimum = min(minimum, 8.0 * field / delta);

        vec3 rPos = position - target;
        if (dot(rPos, rPos) > SCENE_RADIUS) {
            return clamp(minimum, 0.3, 1.0);
        }        
        
        delta += 0.1* field;
        position += field * 0.25 * normal;
    }
    
    return clamp(minimum, 0.3, 1.0);
}

float getAmbientOcclusion(vec3 position, vec3 normal) {
    float brightness = 0.0;
    
    brightness += getDistanceScene(position + 0.5 * OCCLUSION_DELTA * normal, false);
    brightness += getDistanceScene(position + 2.0 * OCCLUSION_DELTA * normal, false);
    brightness += getDistanceScene(position + 4.0 * OCCLUSION_DELTA * normal, false);
    
    brightness = pow(brightness + 0.01, 0.5);    
    return clamp(1.0 * brightness, 0.5, 1.0);
}

vec3 getLightDiffuse(vec3 surfaceNormal, vec3 lightNormal, vec3 lightColor) {
    float power = max(0.0, dot(surfaceNormal, lightNormal));
    return power * lightColor;
}

vec3 getLightSpecular(vec3 reflectionNormal, vec3 viewNormal, vec3 lightColor, float power) {
    return pow(max(0.0, dot(reflectionNormal, viewNormal)), power) * lightColor;
}

float getFresnelSchlick(vec3 surfaceNormal, vec3 halfWayNormal, float r0) {
    return r0 + (1.0 - r0) * pow(1.0 - dot(surfaceNormal, halfWayNormal), 5.0);
}

vec3 computeLight(vec3 position, vec3 surfaceNormal, Material material) {
    vec3 lightPosition = target + vec3(-2.0, 4.0, 2.0);
    vec3 lightAmbient  = 1.0 * vec3(0.2, 0.2, 0.5);
    vec3 lightColor    = 100.0 * vec3(1.1, 0.9, 0.5);
    
    vec3 lightVector = lightPosition - position;
    float attenuation = 1.0 / dot(lightVector, lightVector);
    if (dot(surfaceNormal, lightVector) <= 0.0) return lightAmbient * material.color;
        
    vec3 lightNormal = normalize(lightVector);
    
#ifdef SHADOW
    if (hitInfo.type == TYPE_OPAQUE) {
        lightColor *= getSoftShadow(position, lightNormal);
    }
#endif
    
    vec3 viewNormal       = normalize(viewer - position);
    vec3 halfWayNormal    = normalize(viewNormal + lightNormal);
	vec3 reflectionNormal = mirrorRay(lightNormal, surfaceNormal);
        
    float fresnelTerm  = getFresnelSchlick(surfaceNormal, halfWayNormal, material.shininess);
    vec3 lightDiffuse  = getLightDiffuse(surfaceNormal, lightNormal, lightColor);    
    vec3 lightSpecular = getLightSpecular(reflectionNormal, viewNormal, lightColor, 16.0);
    
    float brightness = getAmbientOcclusion(position, surfaceNormal);
    
    return brightness * (
        lightAmbient + attenuation * (
            material.opacity * lightDiffuse + fresnelTerm * lightSpecular
        )
    ) * material.color;
}

vec3 traceRay(vec3 position, vec3 normal) {
   	vec3 colorOutput = vec3(0.0);
   	vec3 rayColor = vec3(1.0);
    
    float fogAccum = 0.0;
    
  	int mode = MODE_OUTSIDE;
   	for(int hitCount = 0; hitCount < MAX_HIT_COUNT; hitCount++) {
        hitInfo.type = TYPE_NONE;
        
        for (int it = 0; it < ITERATION_COUNT; it++) {            
            float field;
            
            if (mode == MODE_OUTSIDE) {
	            field = getDistanceScene(position, true);            
                fogAccum += abs(field);
                if (field < DISTANCE_EPSILON) break;
            }
            else {
				field = getDistanceSceneTransparent(position, true);
                if (field > DISTANCE_EPSILON) break;
            }

            vec3 rPos = position - target;
            if (dot(rPos, rPos) > SCENE_RADIUS) {
                hitInfo.type = TYPE_NONE;
                break;
            }

            float march = max(DISTANCE_EPSILON, abs(field));
            position = position + STEP_SCALE * march * normal;
        }

     	if (hitInfo.type == TYPE_OPAQUE) {
    		colorOutput += rayColor * computeLight(
                position, 
                getNormalOpaque(position),
                hitInfo.material
            );
            
            break;
        } 
        else if (hitInfo.type == TYPE_TRANSPARENT) {
            vec3 surfaceNormal = getNormalTransparent(position);
            if (mode == MODE_INSIDE) surfaceNormal = -surfaceNormal;
            
            colorOutput += 0.02 * rayColor * computeLight(
                position, 
                surfaceNormal,
                hitInfo.material
            );
		    
            rayColor *= hitInfo.material.color;
            normal = refract(normal, surfaceNormal, N1 / N2);
            
            if (mode == MODE_INSIDE) {
            	if (dot(normal, surfaceNormal) < 0.0) mode = MODE_OUTSIDE;
                else                                  mode = MODE_INSIDE;
            }
            else mode = MODE_INSIDE;
        }
        else {
            break;
        }
 	}
	        
    vec3 dist = position - target;
    return vec3(1.0 / (dot(dist, dist) * 0.001 + 1.0)) * colorOutput;
}

vec3 createRayNormal(vec3 origo, vec3 target, vec3 up, vec2 plane, float fov, vec3 displacement) {
    vec3 axisZ = normalize(target - origo);
	vec3 axisX = normalize(cross(axisZ, up));
    vec3 axisY = cross(axisX, axisZ);

    origo += mat3(axisX, axisY, axisZ) * -displacement;
    
	vec3 point = target + fov * length(target - origo) * (plane.x * axisX + plane.y * axisY);
    return normalize(point - origo);
}

void main() {
    float cosCamera = cos(2.4 + 2.0 * cos(0.4 * iTime) + 0.0 * (fragCoord.y / iResolution.y));
    float sinCamera = sin(2.4 + 2.0 * cos(0.4 * iTime) + 0.0 * (fragCoord.y / iResolution.y));
    
    clock = mod(1.0 * iTime, 20.0 * PI);
    
    vec3 m = vec3(1.6 * -clock, 0.0, 1.0);
    
    float rot = 3.0 + 0.1 * cos(iTime);
    
    viewer = m + vec3(rot * cosCamera, 2.0 * sin(0.4 * iTime) + 2.5, rot * sinCamera);
    target = m + vec3(0.0, 0.5, 0.0);
    
    
    vec3 displacement = vec3(0.0);
    
    // Stereo.
    // if (mod(fragCoord.y, 2.0) > 1.0) displacement.x += 0.02; 
	// else                                displacement.x -= 0.02;
    
    vec3 normal = createRayNormal(
        viewer,
        target,
        vec3(0.0, 1.0, 0.0),
        (fragCoord.xy - 0.5 * iResolution.xy) / iResolution.y,
        1.6,
        displacement
    );
    
    hitInfo.type = TYPE_NONE;
        
    //mat2 r = getRotation(0.25 * length(fragCoord.xy - 0.5 * iResolution.xy) / iResolution.y);
    vec3 color = traceRay(viewer, normal);
    //color.rb = r * color.rg;
   
    fragColor = vec4(color, 1.0);
}

)";

const char* blue = R"(

#ifdef GL_ES
precision highp float;
#endif

uniform float time;
uniform vec2 resolution;

float field(in vec3 p) {
float strength = 7. + .03 * log(1.e-6 + fract(sin(time) * 4373.11));
float accum = 0.;
float prev = 0.;
float tw = 0.;
for (int i = 0; i < 32; ++i) {
float mag = dot(p, p);
p = abs(p) / mag + vec3(-.5, -.4, -1.5);
float w = exp(-float(i) / 7.);
	
accum += w * exp(-strength * pow(abs(mag - prev), 2.3));
tw += w;  prev = mag;
}
return max(0., 5. * accum / tw - .7);
}

void main() {
vec2 uv = 2. * gl_FragCoord.xy / resolution.xy - 1.;
vec2 uvs = uv * resolution.xy / max(resolution.x, resolution.y);
vec3 p = vec3(uvs / 4., 0) + vec3(1., -1.3, 0.);
p += .2 * vec3(sin(time / 16.), sin(time / 12.), sin(time / 128.));
float t = field(p);
float v = (1. - exp((abs(uv.x) - 1.) * 6.)) * (1. - exp((abs(uv.y) - 1.) * 6.));
gl_FragColor = mix(.4, 1., v) * vec4(0.8 * t * t * t, 1.4 * t * t, t, 1.0);
}

)";

const char* lines2 = R"(

#ifdef GL_ES
precision mediump float;
#endif
// mods by dist, shrunk slightly by @danbri

uniform float time;
uniform vec2 mouse, resolution;
void main() {
	vec2 uPos = ( gl_FragCoord.xy / resolution.xy );//normalize wrt y axis
	uPos -= .5;
	vec3 color = vec3(0.0);
	for( float i = 0.; i <20.; ++i ) {
		uPos.y += sin( uPos.x*(i) + (time * i * i * .1) ) * 0.15;
		float fTemp = abs(1.0 / uPos.y / 500.0);
		//vertColor += fTemp;
		color += vec3( fTemp*(8.0-i)/7.0, fTemp*i/10.0, pow(fTemp,1.0)*1.5 );
	}
	gl_FragColor = vec4(color, 10.0);
}

)";

const char* trinity = R"(

// Trinity
// By: Brandon Fogerty
// bfogerty at gmail dot com
// xdpixel.com
 
 
//"For God so loved the world, that he gave his only Son, 
// that whoever believes in him should not perish but have eternal life." (John 3:16)
    
// "Go therefore and make disciples of all nations, baptizing them in 
// the name of the Father and of the Son and of the Holy Spirit, 
// teaching them to observe all that I have commanded you. 
// And behold, I am with you always, to the end of the age." - King Jesus (Matthew 28:19-20)
 
#ifdef GL_ES
precision mediump float;
#endif
 
#extension GL_OES_standard_derivatives : enable
 
uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;
 
 
 
#define Resolution              resolution
#define Time                    time
 
#define HorizontalAmplitude     0.30
#define VerticleAmplitude       0.20
#define HorizontalSpeed         0.90
#define VerticleSpeed           1.50
#define ParticleMinSize         1.76
#define ParticleMaxSize         1.61
#define ParticleBreathingSpeed      0.30
#define ParticleColorChangeSpeed    0.70
#define ParticleCount           2.0
#define ParticleColor1          vec3(9.0, 5.0, 3.0)
#define ParticleColor2          vec3(1.0, 3.0, 9.0)
 
 
float hash( float x )
{
    return fract( sin( x ) * 43758.5453 );
}
 
float noise( vec2 uv )  // Thanks Inigo Quilez
{
    vec3 x = vec3( uv.xy, 0.0 );
    
    vec3 p = floor( x );
    vec3 f = fract( x );
    
    f = f*f*(3.0 - 2.0*f);
    
    float offset = 57.0;
    
    float n = dot( p, vec3(1.0, offset, offset*2.0) );
    
    return mix( mix(    mix( hash( n + 0.0 ),       hash( n + 1.0 ), f.x ),
                        mix( hash( n + offset),     hash( n + offset+1.0), f.x ), f.y ),
                mix(    mix( hash( n + offset*2.0), hash( n + offset*2.0+1.0), f.x),
                        mix( hash( n + offset*3.0), hash( n + offset*3.0+1.0), f.x), f.y), f.z);
}
 
float snoise( vec2 uv )
{
    return noise( uv ) * 2.0 - 1.0;
}
 
 
float perlinNoise( vec2 uv )
{   
    float n =       noise( uv * 1.0 )   * 128.0 +
                noise( uv * 2.0 )   * 64.0 +
                noise( uv * 4.0 )   * 32.0 +
                noise( uv * 8.0 )   * 16.0 +
                noise( uv * 16.0 )  * 8.0 +
                noise( uv * 32.0 )  * 4.0 +
                noise( uv * 64.0 )  * 2.0 +
                noise( uv * 128.0 ) * 1.0;
    
    float noiseVal = n / ( 1.0 + 2.0 + 4.0 + 8.0 + 16.0 + 32.0 + 64.0 + 128.0 );
    noiseVal = abs(noiseVal * 2.0 - 1.0);
    
    return  noiseVal;
}
 
float fBm( vec2 uv, float lacunarity, float gain )
{
    float sum = 0.0;
    float amp = 7.0;
    
    for( int i = 0; i < 2; ++i )
    {
        sum += ( perlinNoise( uv ) ) * amp;
        amp *= gain;
        uv *= lacunarity;
    }
    
    return sum;
}
 
vec3 particles( vec2 pos )
{
    
    vec3 c = vec3( 0, 0, 0 );
    
    float noiseFactor = fBm( pos, 0.01, 0.1);
    
    for( float i = 1.0; i < ParticleCount+1.0; ++i )
    {
        float cs = cos( time * HorizontalSpeed * (i/ParticleCount) + noiseFactor ) * HorizontalAmplitude;
        float ss = sin( time * VerticleSpeed   * (i/ParticleCount) + noiseFactor ) * VerticleAmplitude;
        vec2 origin = vec2( cs , ss );
        
        float t = sin( time * ParticleBreathingSpeed * i ) * 0.5 + 0.5;
        float particleSize = mix( ParticleMinSize, ParticleMaxSize, t );
        float d = clamp( sin( length( pos - origin )  + particleSize ), 0.0, particleSize);
        
        float t2 = sin( time * ParticleColorChangeSpeed * i ) * 0.5 + 0.5;
        vec3 color = mix( ParticleColor1, ParticleColor2, t2 );
        c += color * pow( d, 10.0 );
    }
    
    return c;
}
 
 
float line( vec2 a, vec2 b, vec2 p )
{
    vec2 aTob = b - a;
    vec2 aTop = p - a;
    
    float t = dot( aTop, aTob ) / dot( aTob, aTob);
    
    t = clamp( t, 0.0, 1.0);
    
    float d = length( p - (a + aTob * t) );
    d = 1.0 / d;
    
    return clamp( d, 0.0, 1.0 );
}
 
 
void main( void ) {
 
    float aspectRatio = resolution.x / resolution.y;
    
    vec2 uv = ( gl_FragCoord.xy / resolution.xy );
    
    vec2 signedUV = uv * 2.0 - 1.0;
    signedUV.x *= aspectRatio;
 
    float freqA = mix( 0.4, 1.2, sin(time + 30.0) * 0.5 + 0.5 );
    float freqB = mix( 0.4, 1.2, sin(time + 20.0) * 0.5 + 0.5 );
    float freqC = mix( 0.4, 1.2, sin(time + 10.0) * 0.5 + 0.5 );
    
    
    float scale = 100.0;
    const float v = 70.0;
    vec3 finalColor = vec3( 0.0 );
    
    finalColor = (particles( sin( abs(signedUV) ) ) * length(signedUV)) * 0.20;
    
    float t = line( vec2(-v, -v), vec2(0.0, v), signedUV * scale );
    finalColor += vec3( 8.0 * t, 2.0 * t, 4.0 * t) * freqA;
    t = line( vec2(0.0, v), vec2(v, -v), signedUV * scale );
    finalColor += vec3( 2.0 * t, 8.0 * t, 4.0 * t) * freqB;
    t = line( vec2(-v, -v), vec2(v, -v), signedUV * scale );
    finalColor += vec3( 2.0 * t, 4.0 * t, 8.0 * t) * freqC;
    
 
    scale = scale * 1.2;
    t = line( vec2(0.0, v * 0.2), vec2(0.0, -v * 0.8), signedUV * scale );
    finalColor += vec3( 8.0 * t, 4.0 * t, 2.0 * t) * 0.5;
    
    t = line( vec2(-v * 0.3, -v*0.1), vec2(v * 0.3, -v*0.1), signedUV * scale );
    finalColor += vec3( 8.0 * t, 4.0 * t, 2.0 * t) * 0.4;
 
    gl_FragColor = vec4( finalColor, 1.0 );
}

)";

const char* storm = R"(

//---------------------------------------------------------
// Shader:   StormyFlight2.glsl by S.Guillitte in 2015-02-24
//           Improved version of the lightning bolt effect.
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.
// original: https://www.shadertoy.com/view/XlsGWS
// Tags:     2d, noise, lightning, thunder
//---------------------------------------------------------

#ifdef GL_ES
precision mediump float;
#endif

//#extension GL_OES_standard_derivatives : enable

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

float hash(float x)
{
  return fract(21654.6512 * sin(385.51 * x));
}
float hash(in vec2 p)
{
  return fract(sin(p.x*15.32+p.y*35.78) * 43758.23);
}

vec2 hash2(vec2 p)
{
  return vec2(hash(p*.754),hash(1.5743*p.yx+4.5891))-.5;
}

vec2 hash2b(vec2 p)
{
  return vec2(hash(p*.754),hash(1.5743*p+4.5476351));
}

vec2 add = vec2(1.0, 0.0);

vec2 noise2(vec2 x)
{
  vec2 p = floor(x);
  vec2 f = fract(x);
  f = f*f*(3.0-2.0*f);
  vec2 res = mix(mix( hash2(p),          hash2(p + add.xy),f.x),
                 mix( hash2(p + add.yx), hash2(p + add.xx),f.x),f.y);
  return res;
}

vec2 fbm2(vec2 x)
{
  vec2 r = vec2(0.0);
  float a = 1.0;
    
  for (int i = 0; i < 8; i++)
  {
    r += abs(noise2(x)+.5 )* a;
    x *= 2.;
    a *= .5;
  }
  return r;
}

mat2 m2 = mat2( 0.80, -0.60, 0.60, 0.80 );

vec2 fbm3(vec2 x)
{
  vec2 r = vec2(0.0);
  float a = 1.0;
  for (int i = 0; i < 6; i++)
  {
    r += m2*noise2((x+r)/a)*a; 
    r =- .8*abs(r);
    a *= 1.7;
  }     
  return r;
}

vec3 storm(vec2 x)
{
  float t = .5*time;
  float st = sin(t), ct = cos(t);
  m2 = mat2(ct,st,-st,ct);
  x = fbm3(x+0.5*time)+2.;
  x *= .35;
        
  float c = length(x);
  c = c*c*c;
  vec3 col=vec3(0.6-.1*x.x,0.7,0.8-.1*x.y)*c*x.y;   
  return clamp(col,0.,1.);
}

float dseg(vec2 ba, vec2 pa)
{
  float h = clamp( dot(pa,ba)/dot(ba,ba), -0.2, 1. );	
  return length( pa - ba*h );
}

float arc(vec2 x, vec2 p, vec2 dir)
{
  vec2 r = p;
  float d = 10.;
  for (int i = 0; i < 5; i++)
  {
    vec2 s = noise2(r+time)+dir;
    d = min(d,dseg(s,x-r));
    r += s;      
  }
  return d*3.0; 
}

float thunderbolt(vec2 x, vec2 tgt)
{
  vec2 r = tgt;
  float d = 1000.;
  float dist = length(tgt-x);
     
  for (int i = 0; i < 19; i++)
  {
    if(r.y > x.y+.5)break;
    vec2 s = (noise2(r+time)+vec2(0.,.7))*2.;
    dist = dseg(s,x-r);
    d = min(d,dist);
    r += s;
    if(i-(i/5)*5==0)
    {
      if(i-(i/10)*10==0)
           d = min(d,arc(x,r,vec2( .3,.5)));
      else d = min(d,arc(x,r,vec2(-.3,.5)));
    }
  }
  return exp(-5.*d)+.2*exp(-1.*dist);
}

void main()
{
  vec2 p = 2.0*gl_FragCoord.xy / resolution.yy - 1.0;
  vec2 tgt = vec2(1., -8.);
  float c = 0.;
  vec3 col = vec3(0.);
  float t = hash(floor(5.*time));
  tgt += 8.*hash2b(tgt+t);
  if(hash(t+2.3) > 0.8)
  {
    c = thunderbolt(p*10.+2.*fbm2(5.*p),tgt);	
    col += clamp(1.7*vec3(0.8,.7,.9)*c,0.,1.);	
  }
	col.r *= 2.0;
	col.g *= 3.0;
	col.b *= 7.0;
	
  gl_FragColor = vec4(col, 1.0);
}

)";

const char* curves = R"(

#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

void main()
{
    vec2 r = resolution,
    o = gl_FragCoord.xy - r/2.;
    o = vec2(length(o) / r.y - .4, atan(o.y,o.x));    
    vec4 s = .1*cos(1.618*vec4(0,1,2,3) + time + o.y + sin(o.y) * sin(time)*2.),
    e = s.yzwx, 
    f = min(o.x-s,e-o.x);
    gl_FragColor = dot(clamp(f*r.y,0.,1.), 40.*(s-e)) * (s-.1) - f;
}

)";

const char* waves = R"(

#ifdef GL_ES
precision mediump float;
#endif

uniform float time;
vec2 mouse = vec2(20, 0);
uniform vec2 resolution;

// "Seascape" by Alexander Alekseev aka TDM - 2014
// License Creative Commons Attribution-NonCommercial-ShareAlike 3.0 Unported License.

const int NUM_STEPS = 3;
const float PI	 	= 3.1415;
const float EPSILON	= 1e-3;
float EPSILON_NRM	= 0.;

// sea
const int ITER_gEOMETRY = 20;
const int ITER_FRAGMENT = 4;
const float SEA_HEIGHT = 0.6;
const float SEA_CHOPPY = 5.0;
const float SEA_SPEED = 1.0;
const float SEA_FREQ = 0.16;
const vec3 SEA_BASE = vec3(0.1,0.19,0.22);
const vec3 SEA_WATER_COLOR = vec3(0.8,0.9,0.6);
float SEA_TIME = 11.;
mat2 octave_m = mat2(1.6,1.2,-1.2,1.6);

// math
mat3 fromEuler(vec3 ang) {
	vec2 a1 = vec2(sin(ang.x),cos(ang.x));
    vec2 a2 = vec2(sin(ang.y),cos(ang.y));
    vec2 a3 = vec2(sin(ang.z),cos(ang.z));
    mat3 m;
    m[0] = vec3(a1.y*a3.y+a1.x*a2.x*a3.x,a1.y*a2.x*a3.x+a3.y*a1.x,-a2.y*a3.x);
	m[1] = vec3(-a2.y*a1.x,a1.y*a2.y,a2.x);
	m[2] = vec3(a3.y*a1.x*a2.x+a1.y*a3.x,a1.x*a3.x-a1.y*a3.y*a2.x,a2.y*a3.y);
	return m;
}
float hash( vec2 p ) {
	float h = dot(p,vec2(127.1,311.7));	
    return fract(sin(h)*43758.5453123);
}
float noise( in vec2 p ) {
    vec2 i = floor( p );
    vec2 f = fract( p );	
	vec2 u = f*f*(3.0-2.0*f);
    return -1.0+2.0*mix( mix( hash( i + vec2(0.0,0.0) ), 
                     hash( i + vec2(1.0,0.0) ), u.x),
                mix( hash( i + vec2(0.0,1.0) ), 
                     hash( i + vec2(1.0,1.0) ), u.x), u.y);
}

// lighting
float diffuse(vec3 n,vec3 l,float p) {
    return pow(dot(n,l) * 0.4 + 0.6,p);
}
float specular(vec3 n,vec3 l,vec3 e,float s) {    
    float nrm = (s + 8.0) / (3.1415 * 8.0);
    return pow(max(dot(reflect(e,n),l),0.0),s) * nrm;
}

// sky
vec3 getSkyColor(vec3 e) {
    e.y = max(e.y,0.0);
    vec3 ret;
    ret.x = pow(1.0-e.y,2.0);
    ret.y = 1.0-e.y;
    ret.z = 0.6+(1.0-e.y)*0.4;
    return ret;
}

// sea
float sea_octave(vec2 uv, float choppy) {
    uv += noise(uv);        
    vec2 wv = 1.0-abs(sin(uv));
    vec2 swv = abs(cos(uv));    
    wv = mix(wv,swv,wv);
    return pow(1.0-pow(wv.x * wv.y,0.65),choppy);
}

float map(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz; uv.x *= 0.75;
    
    float d, h = 0.0;    
    for(int i = 0; i < ITER_gEOMETRY; i++) {        
    	d = sea_octave((uv+SEA_TIME)*freq,choppy);
    	d += sea_octave((uv-SEA_TIME)*freq,choppy);
        h += d * amp;        
    	uv *= octave_m; freq *= 1.9; amp *= 0.22;
        choppy = mix(choppy,1.0,0.2);
    }
    return p.y - h;
}

float map_detailed(vec3 p) {
    float freq = SEA_FREQ;
    float amp = SEA_HEIGHT;
    float choppy = SEA_CHOPPY;
    vec2 uv = p.xz; uv.x *= 0.75;
    
    float d, h = 0.0;    
    for(int i = 0; i < ITER_FRAGMENT; i++) {        
    	d = sea_octave((uv+SEA_TIME)*freq,choppy);
    	d += sea_octave((uv-SEA_TIME)*freq,choppy);
        h += d * amp;        
    	uv *= octave_m; freq *= 1.9; amp *= 0.22;
        choppy = mix(choppy,1.0,0.2);
    }
    return p.y - h;
}

vec3 getSeaColor(vec3 p, vec3 n, vec3 l, vec3 eye, vec3 dist) {  
    float fresnel = 1.0 - max(dot(n,-eye),0.0);
    fresnel = pow(fresnel,3.0) * 0.65;
        
    vec3 reflected = getSkyColor(reflect(eye,n));    
    vec3 refracted = SEA_BASE + diffuse(n,l,80.0) * SEA_WATER_COLOR * 0.12; 
    
    vec3 color = mix(refracted,reflected,fresnel);
    
    float atten = max(1.0 - dot(dist,dist) * 0.001, 0.0);
    color += SEA_WATER_COLOR * (p.y - SEA_HEIGHT) * 0.18 * atten;
    
    color += vec3(specular(n,l,eye,60.0));
    
    return color;
}

// tracing
vec3 getNormal(vec3 p, float eps) {
    vec3 n;
    n.y = map_detailed(p);    
    n.x = map_detailed(vec3(p.x+eps,p.y,p.z)) - n.y;
    n.z = map_detailed(vec3(p.x,p.y,p.z+eps)) - n.y;
    n.y = eps;
    return normalize(n);
}

float heightMapTracing(vec3 ori, vec3 dir, out vec3 p) {  
    float tm = .0;
    float tx = 100.0;    
    float hx = map(ori + dir * tx);
    if(hx > 0.0) return tx;   
    float hm = map(ori + dir * tm);    
    float tmid = 0.0;
    for(int i = 0; i < NUM_STEPS; i++) {
        tmid = mix(tm,tx, hm/(hm-hx));                   
        p = ori + dir * tmid;                   
    	float hmid = map(p);
		if(hmid < 0.0) {
        	tx = tmid;
            hx = hmid;
        } else {
            tm = tmid;
            hm = hmid;
        }
    }
    return tmid;
}

// main
void main( void ) {
	EPSILON_NRM = 0.1 / resolution.x;
	SEA_TIME = time*1.0 * SEA_SPEED;
	
	vec2 uv = gl_FragCoord.xy / resolution.xy;
    uv = uv * 2.0 - 1.0;
    uv.x *= resolution.x / resolution.y;    
    float time = time * 0.3 + mouse.x*0.01;
	
	mouse = vec2(0.1, 0);
        
    // ray
    vec3 ang = vec3(3);    
    vec3 ori = vec3(mouse.x*100.0,3.5,5.0);
    vec3 dir = normalize(vec3(uv.xy,-2.0));
	dir.z += length(uv) * 0.15;
    dir = normalize(dir) * fromEuler(ang);
    
    // tracing
    vec3 p;
    heightMapTracing(ori,dir,p);
    vec3 dist = p - ori;
    vec3 n = getNormal(p, dot(dist,dist) * EPSILON_NRM);
    vec3 light = normalize(vec3(0.0,1.0,0.8)); 
             
    // color
    vec3 color = mix(
        getSkyColor(dir),
        getSeaColor(p,n,light,dir,dist),
    	pow(smoothstep(0.0,-0.05,dir.y),0.3));
        
    // post
	gl_FragColor = vec4(pow(color,vec3(0.75)), 1.0);
}

)";

const char* fire = R"(

#extension GL_OES_standard_derivatives : enable

#ifdef GL_ES
precision mediump float;
#endif



uniform float time;
//uniform vec2 mouse;
uniform vec2 resolution;


//
// Description : Array and textureless GLSL 2D/3D/4D simplex 
//							 noise functions.
//			Author : Ian McEwan, Ashima Arts.
//	Maintainer : ijm
//		 Lastmod : 20110822 (ijm)
//		 License : Copyright (C) 2011 Ashima Arts. All rights reserved.
//							 Distributed under the MIT License. See LICENSE file.
//							 https://github.com/ashima/webgl-noise
// 

vec3 mod289(vec3 x) {
	return x - floor(x * (1.0 / 289.0)) * 299.0;
}

vec4 mod289(vec4 x) {
	return x - floor(x * (1.0 / 289.0)) * 289.0;
}

vec4 permute(vec4 x) {
		 return mod289(((x*34.0)+1.0)*x);
}

vec4 taylorInvSqrt(vec4 r)
{
	return 1.79284291400159 - 0.85373472095314 * r;
}

float snoise(vec3 v)
	{ 
	const vec2	C = vec2(1.0/6.0, 1.0/3.0) ;
	const vec4	D = vec4(0.0, 0.5, 1.0, 2.0);

// First corner
	vec3 i	= floor(v + dot(v, C.yyy) );
	vec3 x0 =	 v - i + dot(i, C.xxx) ;

// Other corners
	vec3 g = step(x0.yzx, x0.xyz);
	vec3 l = 1.0 - g;
	vec3 i1 = min( g.xyz, l.zxy );
	vec3 i2 = max( g.xyz, l.zxy );

	//	 x0 = x0 - 0.0 + 0.0 * C.xxx;
	//	 x1 = x0 - i1	+ 1.0 * C.xxx;
	//	 x2 = x0 - i2	+ 2.0 * C.xxx;
	//	 x3 = x0 - 1.0 + 3.0 * C.xxx;
	vec3 x1 = x0 - i1 + C.xxx;
	vec3 x2 = x0 - i2 + C.yyy; // 2.0*C.x = 1/3 = C.y
	vec3 x3 = x0 - D.yyy;			// -1.0+3.0*C.x = -0.5 = -D.y

// Permutations
	i = mod289(i); 
	vec4 p = permute( permute( permute( 
						 i.z + vec4(0.0, i1.z, i2.z, 1.0 ))
					 + i.y + vec4(0.0, i1.y, i2.y, 1.0 )) 
					 + i.x + vec4(0.0, i1.x, i2.x, 1.0 ));

// Gradients: 7x7 points over a square, mapped onto an octahedron.
// The ring size 17*17 = 289 is close to a multiple of 49 (49*6 = 294)
	float n_ = 0.142857142857; // 1.0/7.0
	vec3	ns = n_ * D.wyz - D.xzx;

	vec4 j = p - 49.0 * floor(p * ns.z * ns.z);	//	mod(p,7*7)

	vec4 x_ = floor(j * ns.z);
	vec4 y_ = floor(j - 7.0 * x_ );		// mod(j,N)

	vec4 x = x_ *ns.x + ns.yyyy;
	vec4 y = y_ *ns.x + ns.yyyy;
	vec4 h = 1.0 - abs(x) - abs(y);

	vec4 b0 = vec4( x.xy, y.xy );
	vec4 b1 = vec4( x.zw, y.zw );

	//vec4 s0 = vec4(lessThan(b0,0.0))*2.0 - 1.0;
	//vec4 s1 = vec4(lessThan(b1,0.0))*2.0 - 1.0;
	vec4 s0 = floor(b0)*2.0 + 1.0;
	vec4 s1 = floor(b1)*2.0 + 1.0;
	vec4 sh = -step(h, vec4(0.0));

	vec4 a0 = b0.xzyw + s0.xzyw*sh.xxyy ;
	vec4 a1 = b1.xzyw + s1.xzyw*sh.zzww ;

	vec3 p0 = vec3(a0.xy,h.x);
	vec3 p1 = vec3(a0.zw,h.y);
	vec3 p2 = vec3(a1.xy,h.z);
	vec3 p3 = vec3(a1.zw,h.w);

//Normalise gradients
	//vec4 norm = taylorInvSqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
	vec4 norm = inversesqrt(vec4(dot(p0,p0), dot(p1,p1), dot(p2, p2), dot(p3,p3)));
	p0 *= norm.x;
	p1 *= norm.y;
	p2 *= norm.z;
	p3 *= norm.w;

// Mix final noise value
	vec4 m = max(0.6 - vec4(dot(x0,x0), dot(x1,x1), dot(x2,x2), dot(x3,x3)), 0.0);
	m = m * m;
	return 42.0 * dot( m*m, vec4( dot(p0,x0), dot(p1,x1), 
																dot(p2,x2), dot(p3,x3) ) );
	}

//////////////////////////////////////////////////////////////

// PRNG
// From https://www.shadertoy.com/view/4djSRW
float prng(in vec2 seed) {
	seed = fract (seed * vec2 (5.3983, 5.4427));
	seed += dot (seed.yx, seed.xy + vec2 (21.5351, 14.3137));
	return fract (seed.x * seed.y * 95.4337);
}

//////////////////////////////////////////////////////////////

float PI = 3.1415926535897932384626433832795;

float noiseStack(vec3 pos,int octaves,float falloff){
	float noise = snoise(vec3(pos));
	float off = 1.0;
	if (octaves>1) {
		pos *= 1.0;
		off *= falloff;
		noise = (1.0-off)*noise + off*snoise(vec3(pos));
	}
	if (octaves>2) {
		pos *= 2.0;
		off *= falloff;
		noise = (1.0-off)*noise + off*snoise(vec3(pos));
	}
	if (octaves>3) {
		pos *= 2.0;
		off *= falloff;
		noise = (1.0-off)*noise + off*snoise(vec3(pos));
	}
	return (1.0+noise)/2.0;
}

vec2 noiseStackUV(vec3 pos,int octaves,float falloff,float diff){
	float displaceA = noiseStack(pos,octaves,falloff);
	float displaceB = noiseStack(pos+vec3(3984.293,423.21,5235.19),octaves,falloff);
	return vec2(displaceA,displaceB);
}

void main(  ) {
	vec2 drag = vec2(0.0, 0.0); //mouse.xy;
	vec2 offset = vec2(0.0, 0.0); //mouse.xy;
		//
	float xpart = gl_FragCoord.x/resolution.x;
	float ypart = gl_FragCoord.y/resolution.y;
	//
	float clip = 210.0;
	float ypartClip = gl_FragCoord.y/clip;
	float ypartClippedFalloff = clamp(2.0-ypartClip,0.0,1.0);
	float ypartClipped = min(ypartClip,1.0);
	float ypartClippedn = 1.0-ypartClipped;
	//
	float xfuel = 1.0-abs(2.0*xpart-1.0);//pow(1.0-abs(2.0*xpart-1.0),0.5);
	//
	float timeSpeed = 0.5;
	float realTime = timeSpeed*time;
	//
	vec2 coordScaled = 0.01*gl_FragCoord.xy - 0.02*vec2(offset.x,0.0);
	vec3 position = vec3(coordScaled,0.0) + vec3(1223.0,6434.0,8425.0);
	vec3 flow = vec3(4.1*(0.5-xpart)*pow(ypartClippedn,4.0),-2.0*xfuel*pow(ypartClippedn,64.0),0.0);
	vec3 timing = realTime*vec3(0.0,-1.7,1.1) + flow;
	//
	vec3 displacePos = vec3(1.0,0.5,1.0)*2.4*position+realTime*vec3(0.01,-0.7,1.3);
	vec3 displace3 = vec3(noiseStackUV(displacePos,2,0.4,0.1),0.0);
	//
	vec3 noiseCoord = (vec3(2.0,1.0,1.0)*position+timing+0.4*displace3)/1.0;
	float noise = noiseStack(noiseCoord,3,0.4);
	//
	float flames = pow(ypartClipped,0.3*xfuel)*pow(noise,0.3*xfuel);
	//
	float f = ypartClippedFalloff*pow(1.0-flames*flames*flames,8.0);
	float fff = f*f*f;
	vec3 fire = 1.5*vec3(f, fff, fff*fff);
	//
	// smoke
	float smokeNoise = 0.5+snoise(0.4*position+timing*vec3(1.0,1.0,0.2))/2.0;
	vec3 smoke = vec3(0.3*pow(xfuel,3.0)*pow(ypart,2.0)*(smokeNoise+0.4*(1.0-noise)));
	//
	// sparks
	float sparkGridSize = 30.0;
	vec2 sparkCoord = gl_FragCoord.xy - vec2(2.0*offset.x,190.0*realTime);
	sparkCoord -= 30.0*noiseStackUV(0.01*vec3(sparkCoord,30.0*time),1,0.4,0.1);
	sparkCoord += 100.0*flow.xy;
	if (mod(sparkCoord.y/sparkGridSize,2.0)<1.0) sparkCoord.x += 0.5*sparkGridSize;
	vec2 sparkGridIndex = vec2(floor(sparkCoord/sparkGridSize));
	float sparkRandom = prng(sparkGridIndex);
	float sparkLife = min(10.0*(1.0-min((sparkGridIndex.y+(190.0*realTime/sparkGridSize))/(24.0-20.0*sparkRandom),1.0)),1.0);
	vec3 sparks = vec3(0.0);
	if (sparkLife>0.0) {
		float sparkSize = xfuel*xfuel*sparkRandom*0.08;
		float sparkRadians = 999.0*sparkRandom*2.0*PI + 2.0*time;
		vec2 sparkCircular = vec2(sin(sparkRadians),cos(sparkRadians));
		vec2 sparkOffset = (0.5-sparkSize)*sparkGridSize*sparkCircular;
		vec2 sparkModulus = mod(sparkCoord+sparkOffset,sparkGridSize) - 0.5*vec2(sparkGridSize);
		float sparkLength = length(sparkModulus);
		float sparksGray = max(0.0, 1.0 - sparkLength/(sparkSize*sparkGridSize));
		sparks = sparkLife*sparksGray*vec3(1.0,0.3,0.0);
	}
	//
	gl_FragColor = vec4(max(fire,sparks)+smoke,1.0);
}

)";

const char* octopus = R"(

#ifdef GL_ES
precision highp float;
#endif

#extension GL_OES_standard_derivatives : enable

uniform float time;
uniform vec2 mouse;
uniform vec2 resolution;

void mainImage( out vec4, in vec2 );

void main( void )
{
	mainImage( gl_FragColor, gl_FragCoord.xy );
}

// ------------------------------------------------------------

// https://www.shadertoy.com/view/MdsBz2
#define iGlobalTime time
#define iResolution resolution

// "Octopus!" by Krzysztof Narkowicz @knarkowicz
// License: Public Domain

const float MATH_PI = float( 3.14159265359 );

float saturate( float x )
{
    return clamp( x, 0.0, 1.0 );
}

float Smooth( float x )
{
	return smoothstep( 0.0, 1.0, saturate( x ) );   
}

float Sphere( vec3 p, float s )
{
	return length( p ) - s;
}

float Capsule( vec3 p, vec3 a, vec3 b, float r )
{
    vec3 pa = p - a, ba = b - a;
    float h = clamp( dot(pa,ba)/dot(ba,ba), 0.0, 1.0 );
    return length( pa - ba*h ) - r;
}

float Union( float a, float b )
{
    return min( a, b );
}

float UnionRound( float a, float b, float k )
{
    float h = clamp( 0.5 + 0.5 * ( b - a ) / k, 0.0, 1.0 );
    return mix( b, a, h ) - k * h * ( 1.0 - h );
}

float SubstractRound( float a, float b, float r ) 
{
	vec2 u = max( vec2( r + a, r - b ), vec2( 0.0, 0.0 ) );
	return min( -r, max( a, -b ) ) + length( u );
}

float Displace( float scale, float ampl, vec3 p )
{
    p *= ampl;
	return scale * sin( p.x ) * sin( p.y ) * sin( p.z );
}

float RepeatAngle( inout vec2 p, float n ) 
{
	float angle = 2.0 * MATH_PI / n;
	float a = atan( p.y, p.x ) + angle / 2.0;
	float r = length( p );
	float c = floor( a / angle );
	a = mod( a, angle ) - angle / 2.;
	p = vec2( cos( a ), sin( a ) ) * r;
	return c;
}

void Rotate( inout vec2 p, float a ) 
{
    p = cos( a ) * p + sin( a ) * vec2( p.y, -p.x );
}

float Tentacle( vec3 p, float nth )
{
    p.y += 0.3;
    
    float scale = 1.0 - 2.5 * saturate( abs( p.y ) * 0.25 );    
    
    p.x = abs( p.x );
    
    p -= vec3( 1.0, -0.5, 0.0 );
    Rotate( p.xy, 0.4 * MATH_PI );
    p.x -= sin( p.y * 5.0 - iGlobalTime * 1. ) * 0.1;
    
    float ret = Capsule( p, vec3( 0.0, -1000.0, 0.0 ), vec3( 0.0, 1000.0, 0.0 ), 0.25 * scale );

    p.z = abs( p.z );
    p.y = mod( -cos(iGlobalTime*.6+nth*2.+length(p.xz)*4.)*0.14+p.y + 0.08, 0.16 ) - 0.08;
    p.z -= 0.12 * scale;
    float tent = Capsule( p, vec3( 0.0, 0.0, 0.0 ), vec3( -0.4 * scale, 0.0, 0.0 ), 0.1 * scale );
    
    float pores = Sphere( p - vec3( -0.4 * scale, 0.0, 0.0 ), mix( 0.04, 0.1, scale ) );
    tent = SubstractRound( tent, pores, 0.01 );
  
    ret = UnionRound( ret, tent, 0.05 * scale );
    return ret;
}

float Scene( vec3 p )
{   
    p.z += cos( p.y * 0.2 + iGlobalTime ) * 0.11;
    p.x += sin( p.y * 5.0 + iGlobalTime ) * 0.05;    
    p.y += sin( iGlobalTime * 0.51 ) * 0.1;
    
    Rotate( p.yz, 0.45 + sin( iGlobalTime * 0.53 ) * 0.11 );
    Rotate( p.xz, 0.12 + sin( iGlobalTime * 0.79 ) * 0.09 );
    
    vec3 t = p;
    RepeatAngle( t.xz, 8.0 );
    float ret = Tentacle( t, atan(t.x, t.z)-atan(p.x, p.z) );

    p.z += 0.2;
    p.x += 0.2;
        
    float body = Sphere( p - vec3( -0.0, -0.3, 0.0 ), 0.6 );
    
    t = p;    
    t.x *= 1.0 - t.y * 0.4;
    body = UnionRound( body, Sphere( t - vec3( -0.2, 0.5, 0.4 ), 0.8 ), 0.3 ); 
    
    body += Displace( 0.02, 10.0, p );
   
    ret = UnionRound( ret, body, 0.05 );   
    
    ret = SubstractRound( ret, Sphere( p - vec3( 0.1, -1.0, 0.2 ), 0.4 ), 0.1 );        
    
	return ret;
}

float CastRay( in vec3 ro, in vec3 rd )
{
    const float maxd = 10.0;
    
	float h = 1.0;
    float t = 0.0;
   
    for ( int i = 0; i < 50; ++i )
    {
        if ( h < 0.001 || t > maxd ) 
        {
            break;
        }
        
	    h = Scene( ro + rd * t );
        t += h;
    }

    if ( t > maxd )
    {
        t = -1.0;
    }
	
    return t;
}

vec3 SceneNormal( in vec3 pos )
{
	vec3 eps = vec3( 0.001, 0.0, 0.0 );
	vec3 normal = vec3(
	    Scene( pos + eps.xyy ) - Scene( pos - eps.xyy ),
	    Scene( pos + eps.yxy ) - Scene( pos - eps.yxy ),
	    Scene( pos + eps.yyx ) - Scene( pos - eps.yyx ) );
	return normalize( normal );
}

vec3 WaterKeyColor  = vec3( 0.09, 0.92, 0.98 );
vec3 WaterFillColor = vec3( 0.1, 0.06, 0.28 );

vec3 Water( vec3 rayDir )
{
    Rotate( rayDir.xy, -0.2 ); 
    vec3 color = mix( WaterKeyColor, WaterFillColor, Smooth( -1.2 * rayDir.y + 0.5 ) );
    return color;
}

float Circle( vec2 p, float r )
{
    return ( length( p / r ) - 5.0 ) * r;
}

void BokehLayer( inout vec3 color, vec2 p, vec3 c, float radius )   
{    
    float wrap = 350.0;    
    if ( mod( floor( p.y / wrap + 0.5 ), 2.0 ) == 0.0 )
    {
        p.x += wrap * 0.5;
    }    
    
    vec2 p2 = mod( p + 0.5 * wrap, wrap ) - 0.5 * wrap;
    float sdf = Circle( p2, radius );
    color += c * ( 1.0 - Smooth( sdf * 0.01 ) );
}

void mainImage( out vec4 fragColor, in vec2 fragCoord )
{
	vec2 q = fragCoord.xy / iResolution.xy;
    vec2 p = -1.0 + 2.0 * q;
	p.x *= iResolution.x / iResolution.y;

    vec3 rayOrigin	= vec3( -0.5, -0.5, -4.0 );
	vec3 rayDir 	= normalize( vec3( p.xy, 2.0 ) ); 

    vec3 background = vec3(1.); //Water( rayDir );
      
    p *= 400.0;
    Rotate( p, -0.2 );  
    BokehLayer( background, p + vec2( 125.0, -120.0 * iGlobalTime ), vec3( 0.1 ), 0.5 );
    BokehLayer( background, p * 1.5 + vec2( 546.0, -80.0 * iGlobalTime ), vec3( 0.07 ), 0.25 ); 
    BokehLayer( background, p * 2.3 + vec2( 45.0, -50.0 * iGlobalTime ), vec3( 0.03 ), 0.1 ); 

    vec3 color = background;
	float t = CastRay( rayOrigin, rayDir );
    if ( t > 0.0 )
    {        
        vec3 pos = rayOrigin + t * rayDir;
        vec3 normal = SceneNormal( pos );
        
        float specOcc = Smooth( 0.5 * length( pos - vec3( -0.1, -1.2, -0.2 ) ) );

  
        vec3 c0	= vec3( 0.95, 0.99, 0.43 );
        vec3 c1	= vec3( 0.67, 0.1, 0.05 );
        vec3 c2	= WaterFillColor;
        vec3 baseColor = normal.y > 0.0 ? mix( c1, c0, saturate( normal.y ) ) : mix( c1, c2, saturate( -normal.y ) );
                
        vec3 reflVec = reflect( rayDir, normal );        
        float fresnel = saturate( pow( 1.2 + dot( rayDir, normal ), 5.0 ) );
        color = 0.8 * baseColor + 0.6 * Water( reflVec ) * mix( 0.04, 1.0, fresnel * specOcc );

        float transparency = Smooth( 0.9 + dot( rayDir, normal ) );
        color = mix( color, background, transparency * specOcc );
    }
    
    float vignette = q.x * q.y * ( 1.0 - q.x ) * ( 1.0 - q.y );
    vignette = saturate( pow( 32.0 * vignette, 0.05 ) );
    color *= vignette;
        
    fragColor = vec4( 1.-color, 1.0 );
}

)";
