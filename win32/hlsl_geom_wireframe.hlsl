varying in float3 vertWorldPos[3];
varying in float3 vertWorldNormal[3];

varying out float3 worldNormal;
varying out float3 worldPos;

noperspective varying float3 dist;

struct PS_INPUT
{
	float4 p : SV_POSITION;
	float2 t : TEXCOORD;
	float opacity : OPACITY;
};

// https://msdn.microsoft.com/en-us/library/windows/desktop/bb205122(v=vs.85).aspx
[maxvertexcount(3)]
void main( triangle GS_INPUT input[3], inout TriangleStream<PS_INPUT> OutputStream )
{
	PS_INPUT output;

	float2 WIN_SCALE(1920, 1080);

	// taken from 'Single-Pass Wireframe Rendering'
	float2 p0 = WIN_SCALE * gl_PositionIn[0].xy / gl_PositionIn[0].w;
	float2 p1 = WIN_SCALE * gl_PositionIn[1].xy / gl_PositionIn[1].w;
	float2 p2 = WIN_SCALE * gl_PositionIn[2].xy / gl_PositionIn[2].w;
	float2 v0 = p2 - p1;
	float2 v1 = p2 - p0;
	float2 v2 = p1 - p0;
	float area = abs(v1.x * v2.y - v1.y * v2.x);

	output.dist = float3(area / length(v0), 0, 0);
	output.worldPos = input[0].vertWorldPos;
	output.worldNormal = input[0].vertWorldNormal;
	output.gl_Position = input[0].gl_PositionIn;

	OutputStream.Append( output );

	output.dist = float3(0, area / length(v1), 0);
	output.worldPos = input[1].vertWorldPos;
	output.worldNormal = input[1].vertWorldNormal;
	output.gl_Position = input[1].gl_PositionIn;

	OutputStream.Append( output );

	output.dist = float3(0, 0, area / length(v2));
	output.worldPos = input[2].vertWorldPos;
	output.worldNormal = input[2].vertWorldNormal;
	output.gl_Position = input[2].gl_PositionIn;

	OutputStream.Append( output );

	//EndPrimitive();
}
