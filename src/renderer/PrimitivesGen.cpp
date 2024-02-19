#include "PrimitivesGen.hpp"

std::vector<CubeVertex> CubeVertexData()
{
    return
    {
        // Top face
        {{ -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }}, // Top-left 
        {{ -0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }}, // Bottom-left  
        {{  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }}, // Bottom-right
        {{  0.5f,  0.5f,  0.5f }, {  0.0f,  1.0f,  0.0f }}, // Bottom-right                 
        {{  0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }}, // Top-right
        {{ -0.5f,  0.5f, -0.5f }, {  0.0f,  1.0f,  0.0f }}, // Top-left

        // Bottom face    
        {{ -0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }}, // Bottom-right
        {{ -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }}, // Top-right
        {{  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }}, // Bottom-left
        {{  0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }}, // Top-left        
        {{  0.5f, -0.5f,  0.5f }, {  0.0f, -1.0f,  0.0f }}, // Bottom-left
        {{ -0.5f, -0.5f, -0.5f }, {  0.0f, -1.0f,  0.0f }}, // Top-right      

        // Right face
        {{  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }}, // Top-left
        {{  0.5f, -0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }}, // Bottom-left
        {{  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }}, // Bottom-right
        {{  0.5f, -0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }}, // Bottom-right          
        {{  0.5f,  0.5f, -0.5f }, {  1.0f,  0.0f,  0.0f }}, // Top-right      
        {{  0.5f,  0.5f,  0.5f }, {  1.0f,  0.0f,  0.0f }}, // Top-left

        // Left face
        {{ -0.5f, -0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }}, // Bottom-right
        {{ -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }}, // Top-right
        {{ -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }}, // Bottom-left
        {{ -0.5f,  0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }}, // Top-left       
        {{ -0.5f, -0.5f, -0.5f }, { -1.0f,  0.0f,  0.0f }}, // Bottom-left
        {{ -0.5f,  0.5f,  0.5f }, { -1.0f,  0.0f,  0.0f }}, // Top-right

        // Front face
        {{ -0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }}, // Top-left        
        {{ -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }}, // Bottom-left
        {{  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }}, // Top-right
        {{  0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }}, // Bottom-right        
        {{  0.5f,  0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }}, // Top-right
        {{ -0.5f, -0.5f,  0.5f }, {  0.0f,  0.0f,  1.0f }}, // Bottom-left

        // Back face 
        {{ -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }}, // Bottom-left                
        {{ -0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }}, // Top-left
        {{  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }}, // Top-right
        {{  0.5f,  0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }}, // Top-right              
        {{  0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }}, // Bottom-right    
        {{ -0.5f, -0.5f, -0.5f }, {  0.0f,  0.0f, -1.0f }}  // Bottom-left
    };
}
