#version 450

layout(location = 0) in vec2 uv;

layout(location = 0) out vec4 outColor;

#define MAXNUMPEAKS 32

//bounds in the 2D visualiser
const float xmax = 8.0;
const float xmin = -8.0;
const float ymax = 5.0;
const float ymin = -5.0;


#define PI 3.14159265f

// assign a number to each type of peak
const uint PeakQuadratic = 0;
const uint PeakSphere = 1;
const uint PeakInvQuad = 2;
const uint PeakInvAbs = 3;


const float RenderDist = 200.0f;
const float MinRenderDist = 0.5f;

const uint RayTraceMaxDepth = 4; // max times to recursively call a racast.both reflections and refractions

const float rootIterThreshold = 0.02f; // distance apart of two root candidates to consider converged
const uint maxRootfindIters = 20; //number of times to a apply iterative root finding methods befor we give up on convergence

// Determins min normal dot product to not be an outline
const float OutlineThreshold = 0.5;


const vec2 PrimCubeRoot = 0.5f * vec2(-1.0f, sqrt(3.0f)); //for complex method of finding zeros for cubics

const uint HallyIteration = 10;

const float IntersectionPeakRange = 6.0f; // distance from center that peaks are actually rendered



//import the energy and location of each peak
struct Peak {
	uint type;
	vec2 center;
    float energy;
	vec4 color;
};

layout(binding = 0) uniform UniformBufferObject {
    Peak peaks[MAXNUMPEAKS];
    uint numPeaks;
	mat4 view;
	vec2 fov;
	float aspect;
    vec4 skyCol;
    vec4 floorCol;
} ubo;

//presents energy information as the sum of waves at defined points

vec4 lowCol = uvec4(0.6, 0.0, 0.1, 1.0);
vec4 highCol = uvec4(0.0, 0.5, 1.0, 1.0);
vec4 floorCol = uvec4(0.0, 1.0, 0.6, 1.0);

//frequency scaler of way R^2
float c = 7.0;

//peak sharpness/width (0, infty)
//lowe means wider
float width = 3;

vec2 uvToCart(vec2 uv) {
    return mix(vec2(xmin, ymin), vec2(xmax, ymax), uv);
}

//how energy h, is distributed on nearby space. centered at origin
//only 1 peak
float f1(in float h, in float x, in float y) {
	float r2 = x * x + y * y;
	return h * pow(1.0 / (width * r2 + 1.0), 2);
}
// double repeating peaks
float f2(in float h, in float x, in float y) {
	float r2 = x * x + y * y;
	return h * pow(sin(c * sqrt(r2)) * 1.0 / (width * r2 + 1), 2);
	
}

// single repeating peaks
float f3(in float h, in float x, in float y) {
	float r2 = x * x + y * y;
	return h * pow(cos(c * sqrt(r2)) * 1.0 / (width * r2 + 1), 2);
}

//derivatives of those energy on x and y
//vec2 dr(x, y) = vec2(x, y) * inverse(r) -- accounted in following derivatives
vec2 df1(in float h, in float x, in float y) {
	float r2 = x * x + y * y;
	float denom = 1.0 / (width * r2 + 1.0);

	return -4.0 * h * width * vec2(x, y) * denom * denom;
}

vec2 df2(in float h, in float x, in float y) {
	float r2 = x * x + y * y;
	float r = sqrt(r2);
	float denom = 1.0 / (r2 + 1.0);
	float s = 2.0 * h * sin(c * r) * (c * cos(c * r) * (width * r2 + 1.0) - 2.0 * sin(c * r) * width * r) \
		* pow(denom, 3);
	return s / r * vec2(x, y);
}

vec2 df3(in float h, in float x, in float y) {
	float r2 = x * x + y * y;
	float r = sqrt(r2);
	float denom = 1 / (r2 + 1.0);
	float s = 2.0 * h * cos(c * r) * (-c * sin(c * r) * (width * r2 + 1.0) - 2.0 * cos(c * r) * width * r) \
		* pow(denom, 3);
	return s / r * vec2(x, y);
}


// The three above are prooving too hard both to raycast and sdf For now try to get away with quadratics
// A quadratic peak is a surface of the form y = -width(p - p_0) ^2 + height, that is a hill with epicenter p_0
// We want to see if a line cast at a point in a direction intersects with it, and if so, where
// By convention y+ is up
float fQuadratic(in float height, in float width, in vec2 center, in vec2 point) {
	vec2 diff = point - center;
	float r2 = diff.x * diff.x + diff.y * diff.y;
	//ignor negative energies
	return max(-width * r2 + height, 0.0);
}

bool quadraticSolvable(in float a, in float b, in float c) {
	//just check the determinant
	return (b * b) >= (4.0 * a * c);
}
// input where a quadratic evaluates to 0, if it exists
bool quadraticRoots(const float a, const float b, const float c, out vec2 roots){
    if (!quadraticSolvable(a,b,c)) {
		return false;
	}
    //just a line
    if (a == 0.0f) {
        roots.x = roots.y = -c / b;
    } else {
        float sqrtdet = sqrt(b * b - 4.0 * a * c);
        roots.x = (-b - sqrtdet) / (2.0 * a);
        roots.y = (-b + sqrtdet) / (2.0 * a);
    }
    return true;
}

float cubicHalley(const float a, const float b, const float c, const float d, float x) {
    
    for (uint i = 0; i < maxRootfindIters; i++) {
        float x2 = x * x;
        float xLast = x;
        float f = a * x * x2 + b * x2 + c * x + d;
        float df = a * 3.0f * x2 + 2.0f * b * x + c;
        float df2 = a * 6.0f * x + 2.0f * b;

        x -= (2.0f * f * df) /(2.0f * df * df - f * df2);

        if (abs(x - xLast) < rootIterThreshold)                
            break; // x is a sufficiently converged root
    }
    return x;
}

//use binary search to find an interval between [min, max] where f(min) and f(max) have different signs
//assumed not to be at max or min
vec2 cubicBinaryDiffSigns(const float a, const float b, const float c, const float d, const float lo, float hi) {
    float loVal = a * lo * lo * lo + b * lo * lo + c * lo + d;
    float hiVal = a * hi * hi * hi + b * hi * hi + c * hi + d;
    float lastHi = lo;
    while (sign(loVal) != sign(hiVal)) {
        lastHi = hi;
        hi = (lo + hi) / 2.0f;
		hiVal = a * hi * hi * hi + b * hi * hi + c * hi + d;

    }
    return vec2(hi, lastHi);
}

// Guarenteed to be at least one
bool closestCubicRoot(const float a, const float b, const float c, const float d, out float root) {
    //just a quadratic
    if (a == 0.0f) {
        vec2 r2;
        bool result = quadraticRoots(b,c,d, r2);
        if (result) {
            float a = min(r2.x, r2.y);
            float b = max(r2.x, r2.y);
            if (a >0.0f) {
                root = a;
                return true;
            }
            else if (b >0.0f) {
                root = b;
                return true;
            }
            else {
                return false;
            }
        }
        return result;
    }
    // use iteration based methods (Newton/Halleys)
    // considered using Jenkins and traub, but its looks like overkill for a gpu shader
    float da = 3.0f * a;
    float db = 2.0f * b;
    float d2a = 2.0f * da;
    // distance between starting points to give up on root finding
    const float startThreshold = 0.05f;
    const float MaxRenderDistance = 100.0f;

    float startHi = MaxRenderDistance;
    float startLo = 0.0f;

    float hi = MaxRenderDistance;
    float lo = 0.0f;

    // We can limit the search to one interval if we know how many
    float discriminant = a * (18.0f * b * c * d - 4.0f * c * c * c - 27.0f * a * d * d) + b * b * (-4.0f * b * d + c * c);
    if (discriminant < 0.0f) {
        // Only one root.
        if (sign(d) == sign(a * pow(MaxRenderDistance, 3) + b * MaxRenderDistance * MaxRenderDistance + c * MaxRenderDistance + d)) {
            //root is in the negative interval
            return false;
        }
        // We only care if the root in the positive interval (0, MaxRenderDistance]
        vec2 bounds = cubicBinaryDiffSigns(a, b, c, d, 0.0f, MaxRenderDistance);
        // Perform gradient descent / Housing in last period with different signs
        float x = 0.5f * (bounds.x + bounds.y);
        float r1 = cubicHalley(a, b, c, d, x);
        if (r1 > 0.0f) {
            root = r1;
            return true;
        }
        else {
            return false;
        }
    }
    else {
        // Multiple (possibly multi) roots
        // find extreme points.
        // it is assumed that ep.x <= ep.y
        vec2 ep;
        if (!quadraticRoots(da, db, c, ep)) {
            return false; // dont know how you got here
        }
        float ep1 = min(ep.x, ep.y);
        float ep2 = max(ep.x, ep.y);

        if (ep1 > 0.0f && ep2 > 0.0f) {
            // both positive, then do newton on (0, ep1), (ep1, ep2)
            float r1 = cubicHalley(a, b, c, d, ep2 * 0.5);
            if (r1 > 0.0f) {
                root = r1;
            }
            else {
                root = cubicHalley(a, b, c, d, (ep1 + ep1) * 0.5);
            }
            return true;
        }
        // both negative, equivalent to search with one root
        else if (ep1 < 0.0f && ep2 < 0.0f) {
            if (sign(d) == sign(a * pow(MaxRenderDistance, 3) + b * MaxRenderDistance * MaxRenderDistance + c * MaxRenderDistance + d)) {
            //root is in the negative interval
                return false;
            }
            // We only care if the root in the positive interval (0, MaxRenderDistance]
            vec2 bounds = cubicBinaryDiffSigns(a, b, c, d, 0.0f, MaxRenderDistance);
            // Perform gradient descent / Housing in last period with different signs
            float x = 0.5f * (bounds.x + bounds.y);
            float r1 = cubicHalley(a, b, c, d, x);
            if (r1 > 0.0f) {
                root = r1;
                return true;
            }
            else {
                return false;
            }
        }
        // one positive, one negative. Do newton on (ep1,  ep2), (ep2, Max dist)
        else {
            float r1 = cubicHalley(a, b, c, d, 0.5f * (ep1 + ep2));
            if (r1 <= 0.0f) {
                // upper bound for search
                vec2 bounds =cubicBinaryDiffSigns(a, b, c, d, ep2, MaxRenderDistance);
                root = cubicHalley(a, b, c, d, 0.5f * (bounds.x + bounds.y));
            }
            else {
                root = r1;
            }
            return true;
        }
    }
}

float cbrt (float x) {
    return pow(abs(x), 0.333f) * sign(x);
    // https://stackoverflow.com/questions/21490237/trouble-writing-cube-root-function
    float y = 1.0;
    for (uint i = 0; i < 10; i++) {
        y = ((2.0 * y) + (x / (y * y))) / 3.0;
    }
    return y;
}

#define cx_mul(a, b) vec2(a.x*b.x-a.y*b.y, a.x*b.y+a.y*b.x)
#define cx_div(a, b) vec2(((a.x*b.x+a.y*b.y)/(b.x*b.x+b.y*b.y)),((a.y*b.x-a.x*b.y)/(b.x*b.x+b.y*b.y)))
#define cx_modulus(a) sqrt(a.x * a.x + a.y * a.y)
#define cx_conj(a) vec2(a.x, -a.y)
#define cx_arg(a) atan(a.y, a.x)
#define cx_sin(a) vec2(sin(a.x) * cosh(a.y), cos(a.x) * sinh(a.y))
#define cx_cos(a) vec2(cos(a.x) * cosh(a.y), -sin(a.x) * sinh(a.y))

vec2 cx_sqrt(vec2 a) {
    float r = sqrt(a.x * a.x + a.y * a.y);
    float rpart = sqrt(0.5*(r+a.x));
    float ipart = sqrt(0.5*(r-a.x));
    if (a.y < 0.0) ipart = -ipart;
    return vec2(rpart,ipart);
}

vec2 cx_tan(vec2 a) {return cx_div(cx_sin(a), cx_cos(a)); }

vec2 cx_log(vec2 a) {
    float rpart = sqrt((a.x*a.x)+(a.y*a.y));
    float ipart = atan(a.y,a.x);
    if (ipart > PI) ipart=ipart-(2.0*PI);
    return vec2(log(rpart),ipart);
}
// mine
vec2 cx_acos(vec2 a){
    return cx_mul(vec2(0, -1), cx_log(a + cx_mul(vec2(0.0, 1.0), cx_sqrt(vec2(1.0f, 0.0f) - cx_mul(a, a)))));
} 

vec2 cx_cbrt(vec2 a) {
    float ang = atan(a.y, a.x) / 3.0f;
    float r = cbrt(length(a));
    float real = cos(ang);
    float im = sin(ang);
    return vec2(r * real, r * im);
}

vec2 cx_mobius(vec2 a) {
    vec2 c1 = a - vec2(1.0,0.0);
    vec2 c2 = a + vec2(1.0,0.0);
    return cx_div(c1, c2);
}

// end copypaste

//same as above but with complex numbers
bool closestCubicRoot2(const float a, const float b, const float c, const float d, out float root) {
    //just a quadratic
    vec2 r2;
        bool result = quadraticRoots(b,c,d, r2);
        if (result) {
            float a = min(r2.x, r2.y);
            float b = max(r2.x, r2.y);
            if (a >0.0f) {
                root = a;
                return true;
            }
            else if (b >0.0f) {
                root = b;
                return true;
            }
            else {
                return false;
            }
        }
        return result;

    vec2 p = vec2((3.0f * a * c - b * b) / (3.0f * a * a), 0.0f);
    vec2 q = vec2((2.0f * b * b * b - 9.0f * a * b * c + 27.0f * a * a * d) / (27.0f * a * a * a), 0.0f);
    
    float discriminant = -(4.0f * p.x * p.x * p.x + 27.0f * q.x * q.x);
    if (discriminant  < 0.0f) {
        // one real root
        float t = cbrt(-q.x * 0.5f +sqrt(q.x * q.x * 0.25f + p.x * p.x * p.x / 27.0f)) + cbrt(-q.x * 0.5f - sqrt(q.x * q.x *  0.25f + p.x * p.x * p.x / 27.0f));
        float undepressed = t - b / (3.0f * a);
        if (undepressed < 0.0f) {
            return false;
        }
        root = undepressed;
        return true;
        
    }
    else {
        // multiple roots
        float currSmallest = RenderDist;
        for (int i = 0; i < 3; i++) {
            vec2 inAcos = cx_mul(1.5f * cx_div(q, p), cx_sqrt(cx_div(vec2(-3.0, 0.0f), p)));
            vec2 inCos = (0.333333 * cx_acos(inAcos)) - vec2(0.66666 * PI * float(i), 0.0f);
            //std::cout << inAcos.toString() << ", " << inCos.toString() << std::endl;
            vec2 candRoot = cx_mul(2.0 * cx_sqrt(p / -3.0f),  cx_cos(inCos));
            //std::cout << candRoot.toString() << (abs(candRoot.y) == 0.0)<< std::endl;

            if (abs(candRoot.y) == 0.0) {
                float undepressed = candRoot.x - b / (3.0f * a);
                if((undepressed < currSmallest) && (0.0 < undepressed))                                                                                                                                                                                                                    
                    currSmallest = undepressed;
            }   
        }
        if (currSmallest == RenderDist) {
            //no viable roots found
            return false;
        }
        // pick the min fully real root
        root = currSmallest;
    }
    return true;
}
// non geometric method
bool closestCubicRoot3(const float a, const float b, const float c, const float d, out float root) {
    //just a quadratic
    if (abs(a) == 0.0f) {
        vec2 r2;
        bool result = quadraticRoots(b,c,d, r2);
        if (result) {
            float a = min(r2.x, r2.y); 
            float b = max(r2.x, r2.y);
            if (a > 0.0f){
                root = a;
                return true;
            }
            else if (b > 0.0f) {
                root = b;
                return true;
            }
            else {
                return false;
            }
        }
        return false;
    }
    float disc1 = b * b - 3.0f * a * c;
    float disc2 = 2.0f * b * b * b - 9.0 * a * b * c + 27.0f * a * a * d;
    vec2 bigC;
    if (abs(disc1) == 0.0f){
        if (disc1 == disc2) {
            //special case, a(x + b/3a)
            root = -b / (3.0f * a);
            return true;
        }
        else {
            bigC = (vec2(cbrt(disc2), 0.0f));
        }
    }
    else {
        bigC = cx_cbrt(0.5f * (vec2(disc2, 0.0f) - cx_sqrt(vec2(disc2 * disc2, 0.0f) - vec2(4.0f * disc1 * disc1 * disc1, 0.0f))));
    }
    float currSmallest = RenderDist;
    for (int k = 0; k < 3; k++) {
        bigC = cx_mul(PrimCubeRoot, bigC);
        vec2 candRoot = -0.33333f / a * (vec2(b, 0.0f) + bigC + (cx_div(vec2(disc1, 0.0f), bigC)));
        if (abs(candRoot.y) == 0.0f) {
            if (candRoot.x > 0.0f && currSmallest > candRoot.x) {
                currSmallest = candRoot.x;
            }
        }
    }
    if (currSmallest == RenderDist) {
        return false;
    }
    else {
        root = currSmallest;
        return true;
    }
    return false;
}

//Line/Ray intersection checks

// distance to closest ray intersection to a quadratic peak
bool intersectsQuadratic(float height, float width, vec2 p0, vec3 center, vec3 dir, out float root) {
    // A line intersection with a quadtratic question is just another quadratic solving question
    // where the dependent variable is a parameter that moves along the ray
    // see docs for formula working out
    float px = p0.x - center.x;
    float pz = p0.y - center.z;
    float py = height - center.y;
    float a = -width * (dir.x * dir.x + dir.z * dir.z);
    float b = (-2.0f * -width * (dir.x * px +  dir.z * pz) - dir.y);
    float c = -width*(px * px + pz * pz) + py;

    vec2 roots;
    if (quadraticSolvable(a, b, c)) {
        quadraticRoots(a, b, c, roots);
        a = min(roots.x, roots.y);
        b = max(roots.x, roots.y);
        if (a > 0.0f) {
            root = a;
            return true;
        }
        else if (b > 0.0f) {
            root = b;
            return true;
        }
    }
    return false;
}

bool intersectsSphere(vec3 p0, float r, vec3 l0, vec3 dir, out float root) {
    float px = p0.x - l0.x;
    float py = p0.y - l0.y;
    float pz = p0.z - l0.z;

    float a = dir.x * dir.x + dir.y * dir.y + dir.z * dir.z;
    float b = -2.0f * (dir.x*px + dir.y*py + dir.z*pz);
    float c = (px * px + py * py + pz*pz - r*r);

    //boilerplate closest quadratic
    vec2 roots;
    if (quadraticSolvable(a, b, c)) {
        quadraticRoots(a, b, c, roots);
        a = min(roots.x, roots.y);
        b = max(roots.x, roots.y);
        if (a > 0.0f) {
            root = a;
            return true;
        }
        else if (b > 0.0f) {
            root = b;
            return true;
        }
    }
    return false;
}

bool intersectsInvSquarePeak(float height, float width, vec2 p0, vec3 l0, vec3 dir, out float root) {
    vec3 p1 = {
        p0.x - l0.x,
        height - l0.y,
        p0.y - l0.z
    };
    float a = width * dir.y * (dir.x * dir.x + dir.z * dir.z);
    float b = width * (l0.y * (dir.x * dir.x + dir.z * dir.z) - 2.0f * dir.y * (dir.x*p1.x + dir.z*p1.z));
    float c = width * (dir.y * (p1.x * p1.x + p1.z * p1.z) - 2.0f * l0.y * (p1.x * dir.x + p1.z * dir.z)) + dir.y;
    float d = l0.y * (width * (p1.x * p1.x + p1.z * p1.z) + 1.0f) - height;

    // return closestCubicRoot3(a, b, c, d, root);
    // return closestCubicRoot2(a, b, c, d, root);
    return closestCubicRoot(a, b, c, d, root);
}

//Pointy peak asymptotic at y = 0 and is a quadratic solving problem
bool intersectsInvAbsPeak(float height, float width, vec2 p0, vec3 l0, vec3 dir, out float root) {
    vec3 p1 = {
        p0.x - l0.x,
        height - l0.y,
        p0.y - l0.z
    };
    //constraint that s * (ma * t + va - p0a) >= 0, s \in {-1, 1}
    // For possible quadratic problems in total, only 3 intersections can be valid, only 1 is closest
    float currClosest = RenderDist;
    for (float sx = -1.0f; sx < 2.0f; sx += 2.0f) for (float sz = -1.0f; sz < 2.0f; sz += 2.0f) {
        vec2 roots;
        float a = width * dir.y * (sx * dir.x + sz * dir.z);
        float b = dir.y + width * (l0.y * (sx * dir.x + sz * dir.z) - dir.y * (sx * p1.x + sz * p1.z));
        float c = l0.y * (1.0f - width * (sx * p1.x + sz * p1.z)) - p1.y;
        if (quadraticRoots(a, b, c, roots)) {\
            float candRoot = roots.x;
            // check that point fulfills the constraint, in addition to it being sufficiently close
            if ((sx * (dir.x * candRoot - p1.x) >= 0) && (sz * (dir.z * candRoot - p1.z) >= 0)
                    && (candRoot > 0.0) && (candRoot < currClosest)) {
                currClosest = candRoot;
            }
            
            candRoot = roots.y;
            if ((sx * (dir.x * candRoot - p1.x) >= 0) && (sz * (dir.z * candRoot - p1.z) >= 0)
                    && (candRoot > 0.0) && (candRoot < currClosest)) {
                currClosest = candRoot;
            }
        }
        
    }
    
    
    if (currClosest == RenderDist) {
        return false;
    }
    else {
        root = currClosest;
        return true;
    }
}

bool intersectsPlane(vec3 p0, vec3 pNorm, vec3 l0, vec3 ldir, out float root) {
	//Do not count ANY parralel lines, even if they overlap with the plane
	if (dot(ldir, pNorm) == 0.0f) {
		return false;
	}
	// reduce to a linear equation with 1 parameter and solve
	vec3 p1 = p0 - l0;

	float m = dot(ldir, pNorm);
	float c = dot(p1, pNorm);
	root = c / m;

	return true;
}

//this plane is used so often in might as well be its own function
bool intersectsFloor(vec3 l0, vec3 ldir, out float root) {
	return intersectsPlane(vec3(0,0,0), vec3(0,1,0), l0, ldir, root);
}

//checks that line intersects a vertical cylinder
bool intersectsBoundingCylinder(float radius, vec2 p0, vec3 l0, vec3 dir) {
    // formalate the question as a quadratic problem, then see if it has a solution 
    // (not what that solution is). This avoids us to only use addition, subtration and multiplication.
    // which is fast
    vec2 p1 = p0 - l0.xz;

    float a = dir.x * dir.x + dir.z * dir.z;
    float b = -2.0f * (dir.x * p1.x + dir.z * p1.y);
    float c = p1.x * p1.x + p1.y * p1.y - radius * radius;
    // we can check that the intersection (if it exists) is in front of the ray using a dot product
    // between the diff from the eye to the center of the circle and the direction vector
    float product = p1.x * dir.x + p1.y * dir.z;
    return quadraticSolvable(a,b,c);
}


//normal calculation functions
//point of normal is vec3 but the y value may be ignored if point is not expected to land directly on the curve
vec3 quadraticPeakNormal(float height, float width, vec2 p0, vec3 at) {
    vec2 diff = at.xz - p0;
    return normalize(vec3(width * diff.x, 1.0f, width * diff.y));
}

vec3 spherePeakNormal(vec3 p0, vec3 at) {
    return normalize(p0 - at);
}

vec3 invSquarePeakNormal(float height, float width, vec2 p0, vec3 at) {
    vec2 diff = at.xz - p0;
    float r2 = dot(diff, diff);
    return normalize(vec3(-diff.x * height * width, 0.5f * (r2+1.0f) * (r2+1.0f), -diff.y * height * width));
}

vec3 invAbsPeakNormal(float height, float width, vec2 p0, vec3 at) {
    vec2 diff = at.xz - p0;
    //s's multiply the diffs to make them positive.
    vec2 s = sign(diff);
    float denom = width * (dot(s, diff) + 1.0f);
    return normalize(vec3(-width * s.x * height, denom * denom, -width * s.y * height));
}

/*
3d renders of a plane which are the sum of severar instances of functions on the x, z coords (y down by convension).
Initially a camera is at (0,0,0) look at -1 in the z direction. It is transformed by the uniform view in the ubo.
The Widths of the projection feild of view are also stored as uniforms fov and normalised with uniform aspect
*/

//normalized direction of ray in 3d renders at this fragment given a view and projection transform
void calcRay(in vec2 uv, in mat4 view, in vec2 fov, in float aspect, out vec3 center, out vec3 dir) {
	// scale the uv by aspect to keep the image consitent upen different aspect ratios.
	vec2 fragcoord = 2.0 * (uv - 0.5) * vec2(aspect, -1.0);// +y is up, screen might not be square

	//use it to get dir for this fragment at identity position. Euler rotate (yxz) the identity dierection of (0,0,-1)
	vec2 ang = mix(-fov, fov, uv) * vec2(aspect, 1.0f);
	vec2 sins = sin(ang);
	vec2 coss = cos(ang);
	//mats constructed by columns, then rows
	mat3 rotx = mat3(vec3(1, 0, 0), vec3(0, coss.y, sins.y), vec3(0, -sins.y, coss.y));
	mat3 roty = mat3(coss.x, 0, -sins.x, 0, 1, 0, sins.x, 0, coss.x);
	dir = roty*rotx*vec3(0,0,-1);
	dir = normalize(vec3(tan(fov/2) * fragcoord,-1));

	// camera center is translated origin
	center = (view * vec4(0,0,0,1)).xyz;

	// at identity ray is (0,0,-1), so just transform that to get the endpoint for the ray vector component
	dir = normalize((view * vec4(dir, 1)).xyz - center);
	//dir = vec3(0,0,-1);


}


// called recursively to the color of a raycast
vec4 rayCast(in vec3 viewCenter, in vec3 dir) {

    // closest intersection
	float closestInt = RenderDist; //current closest distance from ray to a peak;
    int closestIntInd = -2; // object id of the closest intersection, so we can calculate the normal. defaults to nothing
	vec4 outCol = vec4(0.0f);
    float h = 2.0f;
    float weight = 1.0f;
    int lastIntInd = -3; // do not intersect with the same surface you bounced off from
    for (uint depth = 0; depth < RayTraceMaxDepth; depth++) {
        // We need to check collision with all objects
	    for (int i = 0; i < ubo.numPeaks; i++) {
		    Peak peak = ubo.peaks[i];
		    // Perform intersection check based on peak type
            bool result = false;
            float root;
            // only perform intersection test if it intersects with a bounding cylinder
            if ((peak.type == PeakSphere) || intersectsBoundingCylinder(IntersectionPeakRange, peak.center, viewCenter, dir)) {
                switch (peak.type) {
                    case PeakQuadratic:
                        result = intersectsQuadratic(h * peak.energy, width, peak.center, viewCenter, dir, root);
                        break;
                    case PeakSphere:
                        //width stors y value for spheres
                        vec3 center = vec3(peak.center.x, width, peak.center.y);
                        result = intersectsSphere(center, peak.energy, viewCenter, dir, root);
                        break;
                    case PeakInvQuad:
                        result = intersectsInvSquarePeak(h * peak.energy, width, peak.center, viewCenter, dir, root);
                        break;
                    case PeakInvAbs:
                        result = intersectsInvAbsPeak(h * peak.energy, width, peak.center, viewCenter, dir, root);
                        break;
                }
            }
            // if (intersectsInvSquarePeak(h * peak.energy, width, peak.center, viewCenter, dir, root)) {
            if (result) {
                //value will always be greater than 0.0f
                if (root > 0.0f && root < closestInt && i != lastIntInd) {
                    // check that the intersection point is in range
                    vec3 at = (viewCenter + (root * dir));
                    vec2 intDiff = at.xz - peak.center;
                    if (length(intDiff) < IntersectionPeakRange) {
                        closestInt = root;
                        closestIntInd = i;
                    }
                }
            }
	    }
	    //floor check 
	    float interFloorAt;
	    if (intersectsFloor(viewCenter, dir, interFloorAt) && -1 != lastIntInd) {
		    if (interFloorAt > 0.0f && interFloorAt < closestInt) {
			    closestInt = interFloorAt;
                closestIntInd = -1;
		    }
	    }
        lastIntInd = closestIntInd;
        // determine the color and normal at that Intersection
        if (closestIntInd == -2) {
            // no intersection, use skybox
            return mix(outCol, ubo.skyCol, weight);
        }
        else if (closestIntInd == -1) {
            // floor, reflect the sky slightly
            vec3 normal = vec3(0.0f, 1.0f, 0.0f);
            viewCenter = viewCenter + closestInt * dir;
            dir = reflect(dir, normal);
            outCol = mix(outCol, ubo.floorCol, weight);
            weight = 0.5f;
        }
        else {
            // hit a peak
            Peak peak = ubo.peaks[closestIntInd];
            vec3 normal = vec3(0, 0, 0);
            vec3 at = viewCenter + closestInt * dir;
            switch (peak.type) { 
                case PeakQuadratic:
                    normal = quadraticPeakNormal(h * peak.energy, width, peak.center, at);
                    break;
                case PeakSphere:
                    normal = spherePeakNormal(vec3(peak.center, width).xzy, at);
                    break;
                case PeakInvQuad:
                    normal = invSquarePeakNormal(h * peak.energy, width, peak.center, at);
                    break;
                case PeakInvAbs:
                    normal = invAbsPeakNormal(h * peak.energy, width, peak.center, at);
                    break;
            }
            // A normal that is too perpendicular to the ray is assumed to be at the edge of the peak >> a good outline
            if (abs(dot(normal, dir)) < OutlineThreshold) {
                outCol = mix(outCol, vec4(0,0,0,1), weight);
                break;
            }

            viewCenter = at;
            dir = normalize(reflect(dir, normal));
            outCol = mix(outCol, peak.color, weight);
            weight = 0.1f;
        }
    }
	//return highCol;
	
	return outCol;
}

// the first ray of raytracing process is created by the uv of the screen
vec4 rayCastingCol(in vec2 uv) {
    vec3 viewCenter;
	vec3 dir;
	calcRay(uv, ubo.view, ubo.fov, ubo.aspect, viewCenter, dir);
    return rayCast(viewCenter, dir);

}

// 3drendering of field as a signed distance field
vec4 rayMarchingCol(in vec2 uv) {
	return vec4(0);
}

void main() {
	vec2 point = uvToCart(uv);
	float w = atan(uv.y-0.5, uv.x-0.5) / (PI);
    //w = cbrt(2) / 1.0f;
    w = 0.1 / 0.0f;
	// outColor = mix(lowCol, highCol, w);
	outColor = rayCastingCol(uv);

}