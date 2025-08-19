#version 450

layout(location = 0) in flat uint inObjectId;

layout(location = 0) out uint outObjectId;

void main() {
    outObjectId = inObjectId;
}
