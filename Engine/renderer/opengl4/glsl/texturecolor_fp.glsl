uniform sampler2D u_DiffuseMap;
uniform vec4      u_Color;

in vec2      var_Tex1;


void main()
{
	gl_FragColor = texture2D(u_DiffuseMap, var_Tex1) * u_Color;
}
