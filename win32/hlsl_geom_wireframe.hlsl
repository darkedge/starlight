#include "hlsl_input.hlsli"

uint infoA[]     = { 0, 0, 0, 0, 1, 1, 2 };
uint infoB[]     = { 1, 1, 2, 0, 2, 1, 2 };
uint infoAd[]    = { 2, 2, 1, 1, 0, 0, 0 };
uint infoBd[]    = { 2, 2, 1, 2, 0, 2, 1 };

//float4 Viewport; // ???
float4 Viewport = float4(1920, 1080, 0, 0);

float2 projToWindow(in float4 pos)
{
    return float2(  Viewport.x*0.5*((pos.x/pos.w) + 1) + Viewport.z,
                    Viewport.y*0.5*(1-(pos.y/pos.w)) + Viewport.w );
}

[maxvertexcount(3)]
void main( triangle GS_INPUT input[3], inout TriangleStream<PS_INPUT_WIRE> outStream )
{
    PS_INPUT_WIRE output;

    // Compute the case from the positions of point in space.
    output.Case = (input[0].position.z < 0)*4 + (input[1].position.z < 0)*2 + (input[2].position.z < 0); 

    // If case is all vertices behind viewpoint (case = 7) then cull.
    if (output.Case == 7) return;

    // Shade and colour face just for the "all in one" technique.
    //output.Col = shadeFace(input[0].PosV, input[1].PosV, input[2].PosV);

   // Transform position to window space
    float2 points[3];
    points[0] = projToWindow(input[0].position);
    points[1] = projToWindow(input[1].position);
    points[2] = projToWindow(input[2].position);

    // If Case is 0, all projected points are defined, do the
    // general case computation
    if (output.Case == 0) 
    {
        output.EdgeA = float4(0,0,0,0);
        output.EdgeB = float4(0,0,0,0);

        // Compute the edges vectors of the transformed triangle
        float2 edges[3];
        edges[0] = points[1] - points[0];
        edges[1] = points[2] - points[1];
        edges[2] = points[0] - points[2];

        // Store the length of the edges
        float lengths[3];
        lengths[0] = length(edges[0]);
        lengths[1] = length(edges[1]);
        lengths[2] = length(edges[2]);

        // Compute the cos angle of each vertices
        float cosAngles[3];
        cosAngles[0] = dot( -edges[2], edges[0]) / ( lengths[2] * lengths[0] );
        cosAngles[1] = dot( -edges[0], edges[1]) / ( lengths[0] * lengths[1] );
        cosAngles[2] = dot( -edges[1], edges[2]) / ( lengths[1] * lengths[2] );

        // The height for each vertices of the triangle
        float heights[3];
        heights[1] = lengths[0]*sqrt(1 - cosAngles[0]*cosAngles[0]);
        heights[2] = lengths[1]*sqrt(1 - cosAngles[1]*cosAngles[1]);
        heights[0] = lengths[2]*sqrt(1 - cosAngles[2]*cosAngles[2]);

        float edgeSigns[3];
        edgeSigns[0] = (edges[0].x > 0 ? 1 : -1);
        edgeSigns[1] = (edges[1].x > 0 ? 1 : -1);
        edgeSigns[2] = (edges[2].x > 0 ? 1 : -1);

        float edgeOffsets[3];
        edgeOffsets[0] = lengths[0]*(0.5 - 0.5*edgeSigns[0]);
        edgeOffsets[1] = lengths[1]*(0.5 - 0.5*edgeSigns[1]);
        edgeOffsets[2] = lengths[2]*(0.5 - 0.5*edgeSigns[2]);

        output.Pos =( input[0].position );
        output.uv =( input[0].uv );
        output.EdgeA[0] = 0;
        output.EdgeA[1] = heights[0];
        output.EdgeA[2] = 0;
        output.EdgeB[0] = edgeOffsets[0];
        output.EdgeB[1] = edgeOffsets[1] + edgeSigns[1] * cosAngles[1]*lengths[0];
        output.EdgeB[2] = edgeOffsets[2] + edgeSigns[2] * lengths[2];
        outStream.Append( output );

        output.Pos = ( input[1].position );
        output.uv = ( input[1].uv );
        output.EdgeA[0] = 0;
        output.EdgeA[1] = 0;
        output.EdgeA[2] = heights[1];
        output.EdgeB[0] = edgeOffsets[0] + edgeSigns[0] * lengths[0];
        output.EdgeB[1] = edgeOffsets[1];
        output.EdgeB[2] = edgeOffsets[2] + edgeSigns[2] * cosAngles[2]*lengths[1];
        outStream.Append( output );

        output.Pos = ( input[2].position );
        output.uv = ( input[2].uv );
        output.EdgeA[0] = heights[2];
        output.EdgeA[1] = 0;
        output.EdgeA[2] = 0;
        output.EdgeB[0] = edgeOffsets[0] + edgeSigns[0] * cosAngles[0]*lengths[2];
        output.EdgeB[1] = edgeOffsets[1] + edgeSigns[1] * lengths[1];
        output.EdgeB[2] = edgeOffsets[2];
        outStream.Append( output );

        outStream.RestartStrip();
    }
    // Else need some tricky computations
    else
    {
        // Then compute and pass the edge definitions from the case
        output.EdgeA.xy = points[ infoA[output.Case] ];
        output.EdgeB.xy = points[ infoB[output.Case] ];

		output.EdgeA.zw = normalize( output.EdgeA.xy - points[ infoAd[output.Case] ] ); 
        output.EdgeB.zw = normalize( output.EdgeB.xy - points[ infoBd[output.Case] ] );
		
		// Generate vertices
        output.Pos =( input[0].position );
        output.uv =( input[0].uv );
        outStream.Append( output );
     
        output.Pos = ( input[1].position );
        output.uv = ( input[1].uv );
        outStream.Append( output );

        output.Pos = ( input[2].position );
        output.uv = ( input[2].uv );
        outStream.Append( output );

        outStream.RestartStrip();
    }
}
