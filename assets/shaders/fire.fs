#version 330
in vec2 fragTexCoord;
out vec4 finalColor;

uniform sampler2D texture0; // sprite
uniform float uTime;
uniform float uIntensity; // 0..1
uniform int uMode; // 0 sprite,1 aura
uniform vec2 uSpriteSize; // w,h w pikselach oryginalnych

// prosta hash/ noise
// hash zwraca pojedynczą wartość (poprzednio zwracał vec2 -> błąd konwersji)
float hash(vec2 p){
    vec2 q = vec2(dot(p, vec2(127.1,311.7)), dot(p, vec2(269.5,183.3)));
    float n = sin(q.x + q.y) * 43758.5453123;
    return -1.0 + 2.0*fract(n);
}
float noise(vec2 p){ vec2 i=floor(p); vec2 f=fract(p); vec2 u=f*f*(3.0-2.0*f); return mix(mix(hash(i+vec2(0,0)),hash(i+vec2(1,0)),u.x), mix(hash(i+vec2(0,1)),hash(i+vec2(1,1)),u.x), u.y); }

void main(){
    vec4 base = texture(texture0, fragTexCoord);
    if(uMode==0){ if(base.a < 0.05) discard; }
    // anim percepcyjny ogień
    float t = uTime*1.8;
    vec2 uv = fragTexCoord;
    // przesuw w górę
    float flame = 0.0;
    for(int i=0;i<3;i++){
        float fi=float(i);
        flame += (0.5/(fi+1.0))*noise(uv*vec2(6.0,3.0+fi*2.0)+vec2(0.0,t*(1.0+fi*0.5)));
    }
    flame = clamp(flame,0.0,1.0);
    // gradient pionowy - silniej u góry
    float grad = pow(uv.y, 0.7);
    float glow = flame * grad * uIntensity;
    // kolory ognia
    vec3 fireCol = mix(vec3(0.9,0.2,0.05), vec3(1.0,0.85,0.2), clamp(glow*1.4,0.0,1.0));
    if(uMode==0){
        vec3 col = base.rgb + fireCol * (0.45 + glow*1.35);
        float aura = glow * 0.65;
        float outAlpha = clamp(base.a + aura, 0.0,1.0);
        finalColor = vec4(col, outAlpha);
    } else {
        // Emisja wychodzi z sylwetki: używamy alpha sprite'a jako rdzeń
        float coreMask = smoothstep(0.05,0.35, base.a); // rdzeń wewnątrz postaci
        // Lokalny układ współrzędnych: Y rośnie w górę płomienia (od stóp)
        float y = 1.0 - fragTexCoord.y; // 0 dół, 1 góra
        vec2 center = vec2(0.5,0.5);
        vec2 d2 = fragTexCoord - center;
        float radial = length(d2 * vec2(1.0,1.25));
        // Noise deformujący kształt (języki)
        float t = uTime;
        float n1 = noise(d2*8.0 + vec2(0.0,t*2.2));
        float n2 = noise(d2*17.0 + vec2(4.5,t*3.4))*0.5;
        float deform = n1*0.6 + n2*0.4;
        float radius = 0.60 + deform*0.18; // dynamiczna krawędź
        float shell = smoothstep(radius, radius*0.55, radial);
        // Gradient pionowy – silniejszy dół (gorętszy)
        float vertical = pow(y,0.55);
        // Flicker bazujący na flame + dodatkowy wysoki noise
        float flickerN = noise(d2*9.0 + vec2(7.0,t*5.0));
        float flicker = 0.55 + 0.45*(0.5+0.5*sin(t*6.0 + flickerN*6.2831));
        // Łączna intensywność: rdzeń (alpha postaci) + shell wychodzący
        float intensity = (coreMask*0.65 + shell*0.9) * vertical * flicker * uIntensity;
        intensity = pow(clamp(intensity,0.0,1.6), 1.05);
        // Kolory bardziej wyraziste
        vec3 cCool = vec3(0.35,0.05,0.02);
        vec3 cWarm = vec3(1.05,0.35,0.05);
        vec3 cHot  = vec3(1.25,1.05,0.35);
        float hotMix = pow(intensity,0.85);
        vec3 col = mix(cWarm, cHot, hotMix);
        col = mix(cCool, col, clamp(y*1.2,0.0,1.0));
        // Dodatkowe języki jasności na krawędziach
        float tongues = noise(d2*24.0 + vec2(t*9.0,3.0));
        col += tongues*0.18*hotMix;
        float alpha = intensity * 0.9;
        // Wygaszanie najbardziej odległych fragmentów, by uniknąć prostokątności
        float softEdge = smoothstep(0.95,0.55, radial);
        alpha *= softEdge;
        if(alpha < 0.03) discard;
        finalColor = vec4(col, alpha);
    }
}
