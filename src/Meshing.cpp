#include "Textures.h"

#include "Meshing.h"

#include "InputOutput.h"
#include <iostream>



void Load_OBJ(std::string filename, std::vector<float>& vertex_data, std::vector<float>& normals,
    std::vector<float>& texcoords, std::vector<int>& face_data) {
    // OBJ stores normal/texcoords independently of vertex info
    // So, duplicate each vertex if it has a different normal or texcoord.
    // vertex,normal,tex index -> vertex instance
    std::map<std::tuple<int, int, int>, int> vertex_identity;

    std::vector<float> vertices;      // each vertex gets 3 floats from this list!
    const int cmp_per_vtx = 8;

    std::vector<int>   face_indices;  // temporary for dealing with faces...

    std::ifstream objfile(filename);
    std::string line;
    std::stringstream lineReader;

    int unprocessable_lines = 0;

    float x, y, z, u, v;
    int vID, nID, tID, fullVertexID;
    size_t index, hashIndex;
    std::tuple<int, int, int> full_vertex;

    int root_vID, prev_vID, this_vID;
    int faceCount = 0;

    normals.push_back(0.); normals.push_back(1.); normals.push_back(0.);
    texcoords.push_back(0.); texcoords.push_back(0.);

    while (std::getline(objfile, line)) {

        if (line == "") {
            continue;
        }

        hashIndex = line.find('#');
        if (hashIndex != std::string::npos)
            line = line.substr(0, hashIndex - 1);

        auto words = StringUtil::split(line, ' ', true);
        if (words.size() == 0) continue;

        if (words[0].size() > 1 && words[0][0] == 'v' && words[0][1] == 't') {
            if (words.size() != 3) { unprocessable_lines += 1; continue; }
            texcoords.push_back(std::stof(words[1]));
            texcoords.push_back(std::stof(words[2]));
        }
        else if (words[0].size() > 1 && words[0][0] == 'v' && words[0][1] == 'n') {
            if (words.size() != 4) { unprocessable_lines += 1; continue; }
            normals.push_back(std::stof(words[1]));
            normals.push_back(std::stof(words[2]));
            normals.push_back(std::stof(words[3]));
        }
        else if (words[0][0] == 'v') {
            if (words.size() != 4) { unprocessable_lines += 1; continue; }
            vertices.push_back(std::stof(words[1]));
            vertices.push_back(std::stof(words[2]));
            vertices.push_back(std::stof(words[3]));
        }
        else if (words[0][0] == 'f') {
            faceCount += 1;
            face_indices.clear();

            for (int vxdefID = 1; vxdefID < words.size(); vxdefID++) {
                auto vtxdef = words[vxdefID];
                auto indices = StringUtil::split(vtxdef, '/', false);

                tID = 0;
                nID = 0;
                // NOTE: OBJ indices are 1-indexed.
                // 0 becomes the default for normal and texcoord, but one must be subtracted from the vertex
                switch (indices.size()) {
                case 3:  if (indices[2].size() > 0) nID = std::stof(indices[2]);
                case 2:  if (indices[1].size() > 0) tID = std::stof(indices[1]);
                case 1:  vID = std::stof(indices[0]); vID--;
                }

                full_vertex = { vID,nID,tID };
                auto vtxloc = vertex_identity.find(full_vertex);
                if (vtxloc != vertex_identity.end()) {
                    face_indices.push_back(vtxloc->second);
                }
                else {
                    fullVertexID = vertex_data.size() / cmp_per_vtx;
                    face_indices.push_back(fullVertexID);
                    vertex_identity.insert({ full_vertex, fullVertexID });

                    vertex_data.push_back(vertices[3 * vID + 0]); // x
                    vertex_data.push_back(vertices[3 * vID + 1]); // y
                    vertex_data.push_back(vertices[3 * vID + 2]); // z
                    vertex_data.push_back(normals[3 * nID + 0]); // nx
                    vertex_data.push_back(normals[3 * nID + 1]); // ny
                    vertex_data.push_back(normals[3 * nID + 2]); // nz
                    vertex_data.push_back(texcoords[2 * tID + 0]); // u
                    vertex_data.push_back(texcoords[2 * tID + 1]); // v
                }
            }

            // WARNING: This assumes faces are all convex!
            // WARNING: This will crash if there are less than 2 vertices.

            root_vID = face_indices[0];
            prev_vID = face_indices[1];

            for (int fID = 2; fID < face_indices.size(); fID++) {
                this_vID = face_indices[fID];

                face_data.push_back(root_vID);
                face_data.push_back(prev_vID);
                face_data.push_back(this_vID);

                prev_vID = this_vID;
            }

        }
        else {
            const auto non_whitespace_index = line.find_first_not_of(" \t\n\r");
            if (non_whitespace_index != std::string::npos) {
                unprocessable_lines += 1;
            }
        }
    }

    std::cout << "Loaded mesh with " << (vertices.size() / 3) << " vertices and " << faceCount << " faces." << std::endl;
    std::cout << "\tExpanded to " << (vertex_data.size() / 8) << " vertices." << std::endl;
    std::cout << "\tWith " << unprocessable_lines << " unprocessed lines." << std::endl;
}

s_ptr<MeshData> LoadOBJModel(std::string filename, bool packIt) {
    s_ptr<MeshData> result = std::make_shared<MeshData>();
    result->isPacked = packIt;

    std::vector<float> vertex_data;   // 3+3+2 = 8 components per vertex
    std::vector<float> normals;       // 3 floats per vertex
    std::vector<float> texcoords;     // 2 components per vertex
    std::vector<int>   face_data;     // 3 vertices per face

    Load_OBJ(filename, vertex_data, normals, texcoords, face_data);

    for (size_t i = 0; i < vertex_data.size(); i += 8) {
        vec3 pos(vertex_data[i], vertex_data[i + 1], vertex_data[i + 2]);
        vec3 norm(vertex_data[i + 3], vertex_data[i + 4], vertex_data[i + 5]);
        vec2 uv(vertex_data[i + 6], vertex_data[i + 7]);

        if (result->isPacked) {
            result->vertex_data.push_back({ pos, norm, uv });
        }
        else {
            result->positions.push_back(pos);
            result->normals.push_back(norm);
            result->uvs.push_back(uv);
        }
    }

    for (auto& i : face_data) {
        result->indices.push_back(static_cast<uint32_t>(i));
    }

    return result;
}

std::vector<MaterialData> MaterialData::LoadFromFile(const std::string& filename) {
    std::vector<MaterialData> results;

    std::ifstream mtlIn(filename);
    std::string token;

    while (mtlIn >> token) {
        if (StringUtil::contains(token, "newmtl")) {
            MaterialData mat;
            mat.filename = filename;
            std::string matName;
            mtlIn >> mat.materialName;
            log("Reading material {0}\n", mat.materialName);

            while (mtlIn >> token) {
                if (token == "Ka") {
                    mtlIn >> mat.ambient.x;
                    mtlIn >> mat.ambient.y;
                    mtlIn >> mat.ambient.z;
                }
                else if (token == "Kd") {
                    mtlIn >> mat.diffuse.x;
                    mtlIn >> mat.diffuse.y;
                    mtlIn >> mat.diffuse.z;
                }
                else if (token == "Ks") {
                    mtlIn >> mat.specular.x;
                    mtlIn >> mat.specular.y;
                    mtlIn >> mat.specular.z;
                }

                if (token == "map_Ka") {
                    mtlIn >> mat.ambientTextureFile;
                }
                else if (token == "map_Kd") {
                    mtlIn >> mat.diffuseTextureFile;
                }
                else if (token == "map_Ks") {
                    mtlIn >> mat.specularColorTextureFile;
                }
                else if (token == "map_Ns") {
                    mtlIn >> mat.specularHighlightTextureFile;
                }
                else if (token == "map_d") {
                    mtlIn >> mat.alphaTextureFile;
                }
                else if (token == "map_bump" || token == "bump") {
                    mtlIn >> mat.bumpTextureFile;
                }
                else if (token == "disp") {
                    mtlIn >> mat.displacementTextureFile;
                }
                else if (token == "decal") {
                    mtlIn >> mat.stencilTextureFile;
                }
            }

            auto materialFolder = IO::pathOfFile(filename);

            if (!mat.ambientTextureFile.empty()) {
                mat.ambientTexture = std::make_shared<Texture>(IO::joinPath(materialFolder, mat.ambientTextureFile));
                TextureRegistry::addTexture(mat.ambientTexture);
            }
            if (!mat.diffuseTextureFile.empty()) {
                mat.diffuseTexture = std::make_shared<Texture>(IO::joinPath(materialFolder, mat.diffuseTextureFile));
                TextureRegistry::addTexture(mat.diffuseTexture);
            }
            if (!mat.specularColorTextureFile.empty()) {
                mat.specularColorTexture = std::make_shared<Texture>(IO::joinPath(materialFolder, mat.specularColorTextureFile));
                TextureRegistry::addTexture(mat.specularColorTexture);
            }
            if (!mat.specularHighlightTextureFile.empty()) {
                mat.specularHighlightTexture = std::make_shared<Texture>(IO::joinPath(materialFolder, mat.specularHighlightTextureFile));
                TextureRegistry::addTexture(mat.specularHighlightTexture);
            }
            if (!mat.alphaTextureFile.empty()) {
                mat.alphaTexture = std::make_shared<Texture>(IO::joinPath(materialFolder, mat.alphaTextureFile));
                TextureRegistry::addTexture(mat.alphaTexture);
            }
            if (!mat.bumpTextureFile.empty()) {
                mat.bumpTexture = std::make_shared<Texture>(IO::joinPath(materialFolder, mat.bumpTextureFile));
                TextureRegistry::addTexture(mat.bumpTexture);
            }
            if (!mat.displacementTextureFile.empty()) {
                mat.displacementTexture = std::make_shared<Texture>(IO::joinPath(materialFolder, mat.displacementTextureFile));
                TextureRegistry::addTexture(mat.displacementTexture);
            }
            if (!mat.stencilTextureFile.empty()) {
                mat.stencilTexture = std::make_shared<Texture>(IO::joinPath(materialFolder, mat.stencilTextureFile));
                TextureRegistry::addTexture(mat.stencilTexture);
            }


            results.push_back(mat);
        }
    }

    return results;
}