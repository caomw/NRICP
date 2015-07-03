#include "mesh.h"
#include <math.h>
#include <boost/tokenizer.hpp>

#define EPSILON 0.000001


//TO DO: Fix normals issue (normal count >> vert count?)
//Temp fix: Calculate my own normals when loading mesh

using namespace Eigen;

Mesh::Mesh()
{
    m_vertices = new std::vector<GLfloat>();
    m_vertNormalLines = new std::vector<GLfloat>();
    m_normals = new std::vector<GLfloat>();
    m_texcoords = new std::vector<GLfloat>();
    m_faceIndices = new std::vector<GLuint>();
    m_adjMat = NULL;
    m_D = NULL;
    m_position[0] = 0.0;
    m_position[1] = 0.0;
    m_position[2] = 0.0;
    m_vertCount = 0;
    m_edgeCount = 0;
    m_faceCount = 0;
    m_texCoordCount = 0;
    m_pickedIndex = -1;
    m_wireframe = false;
    m_modified = false;
    m_landmarkVertexIndices = new std::vector<int>();   
    m_segmentationRoot = NULL;
    m_segmentationMode = false;
    m_segmentsIndices = new std::vector<GLuint>();
}

Mesh::~Mesh()
{
 if(m_vertices)
 {
   delete [] m_vertices;
 }

 if(m_vertNormalLines)
 {
   delete [] m_vertNormalLines;
 }

 if(m_normals)
 {
   delete [] m_normals;
 }

 if(m_texcoords)
 {
   delete [] m_texcoords;
 }

 if(m_faceIndices)
 {
   delete [] m_faceIndices;
 }

 if(m_adjMat)
 {
   delete [] m_adjMat;
 }

 if(m_D)
 {
   delete m_D;
 }

 if(m_neighbours)
 {
   delete [] m_neighbours;
 }

 if(m_landmarkVertexIndices)
 {
   delete [] m_landmarkVertexIndices;
 }

 destroySegments(m_segmentationRoot);
}

void Mesh::setVertex(unsigned int _vertNo, Vector3f _value)
{
    unsigned int three_i = 3 * _vertNo;
    unsigned int four_i = 4 * _vertNo;
    m_vertices->at(three_i) = _value[0];
    m_vertices->at(three_i + 1) = _value[1];
    m_vertices->at(three_i + 2) = _value[2];

    if(m_D)
    {
        m_D->coeffRef(_vertNo, four_i) = _value[0];
        m_D->coeffRef(_vertNo, four_i + 1) = _value[1];
        m_D->coeffRef(_vertNo, four_i + 2) = _value[2];
    }
}

bool Mesh::loadMesh(const char *_fileName)
{    
    ifstream in(_fileName, ios::in);
    if (!in)
    {
     printf("Cannot open %s",_fileName);
     return false;
    }

    m_position[0] = 0.0;
    m_position[1] = 0.0;
    m_position[2] = 0.0;

    string line;
    while (getline(in, line))
      {
       if (line.substr(0,2) == "v ")
       {
           istringstream s(line.substr(2));
           vec3 vert;
           s >> vert.v[0];
           s >> vert.v[1];
           s >> vert.v[2];

           m_vertices->push_back((GLfloat)vert.v[0]);
           m_vertices->push_back((GLfloat)vert.v[1]);
           m_vertices->push_back((GLfloat)vert.v[2]);
           m_vertCount++;
           m_position[0] += (GLfloat)vert.v[0];
           m_position[1] += (GLfloat)vert.v[1];
           m_position[2] += (GLfloat)vert.v[2];

           continue;
        }
/*
         if (line.substr(0,2) == "vt")
        {
           istringstream s(line.substr(2));
           vec2 texture;
           s >> texture.v[0];
           s >> texture.v[1];

           m_texcoords->push_back((GLfloat)texture.v[0]);
           m_texcoords->push_back((GLfloat)texture.v[1]);
           m_texCoordCount++;

           continue;
        }

         if (line.substr(0,2) == "vn")
        {
           istringstream s(line.substr(2));
           vec3 normal;
           s >> normal.v[0];
           s >> normal.v[1];
           s >> normal.v[2];

           m_normals->push_back((GLfloat)normal.v[0]);
           m_normals->push_back((GLfloat)normal.v[1]);
           m_normals->push_back((GLfloat)normal.v[2]);

           continue;
         }
*/
        if (line.substr(0,2) == "f ")
        {
            istringstream s(line.substr(2));
            //char* tokens;
            string a;
            string b;
            string c;

            s >> a;
            s >> b;
            s >> c;

            boost::char_separator<char> sep("/");
            boost::tokenizer< boost::char_separator<char> > tokens_a(a, sep);
            boost::tokenizer< boost::char_separator<char> > tokens_b(b, sep);
            boost::tokenizer< boost::char_separator<char> > tokens_c(c, sep);
            //for(tokenizer< char_separator<char> >::iterator beg=tokens.begin(); beg!=tokens.end();++beg)
            //{}
            if(tokens_a.begin()!=tokens_a.end())
            {
             m_faceIndices->push_back((GLuint)(std::atoi((*tokens_a.begin()).c_str()) - 1));
            }

            if(tokens_b.begin()!=tokens_b.end())
            {
             m_faceIndices->push_back((GLuint)std::atoi((*tokens_b.begin()).c_str()) -1);
            }

            if(tokens_c.begin()!=tokens_c.end())
            {
             m_faceIndices->push_back((GLuint)std::atoi((*tokens_c.begin()).c_str()) -1);
            }
            m_faceCount++;

          continue;
        }
     }    

    m_position = m_position/m_vertCount;
    unsigned int three_i;

    for(unsigned int i = 0; i < m_vertCount; i++)
    {
      three_i = 3*i;
      m_vertices->at(three_i) = m_vertices->at(three_i) - m_position[0];
      m_vertices->at(three_i + 1) = m_vertices->at(three_i + 1) - m_position[1];
      m_vertices->at(three_i + 2) = m_vertices->at(three_i + 2) - m_position[2];
    }

    return true;
}

void Mesh::printLandmarkedPoints(const char*_fileName)
{
  ofstream file;
  file.open(_fileName);
  unsigned int size = m_landmarkVertexIndices->size();

  for(unsigned int i=0; i<size; ++i)
  {
   file << m_landmarkVertexIndices->at(i);
   file << "\n";
  }

  file.flush();
  file.close();
}

void Mesh::calculateNormals()
{
  if(m_normals->size()<1)
  {
      for(unsigned int k=0; k<m_vertices->size(); ++k)
      {
       m_normals->push_back(0.0);
      }
  }
  else
  {
    for(unsigned int k=0; k<m_normals->size(); ++k)
    {
     m_normals->at(k) = 0.0;
    }
  }

  unsigned int three_i;
  unsigned short v1;
  unsigned short v2;
  unsigned short v3;
  unsigned int three_v1;
  unsigned int three_v2;
  unsigned int three_v3;

  for (unsigned int i = 0; i < m_faceCount; ++i)
  {
   three_i = 3 * i;
   v1 = m_faceIndices->at(three_i);
   v2 = m_faceIndices->at(three_i + 1);
   v3 = m_faceIndices->at(three_i + 2);
   three_v1 = 3 * v1;
   three_v2 = 3 * v2;
   three_v3 = 3 * v3;

   Vector3f normal(0.0, 0.0, 0.0);
   Vector3f point1(m_vertices->at(three_v1), m_vertices->at(three_v1 + 1), m_vertices->at(three_v1 + 2));
   Vector3f point2(m_vertices->at(three_v2), m_vertices->at(three_v2 + 1), m_vertices->at(three_v2 + 2));
   Vector3f point3(m_vertices->at(three_v3), m_vertices->at(three_v3 + 1), m_vertices->at(three_v3 + 2));
   Vector3f edge1 = 10*(point2 - point1);
   Vector3f edge2 = 10*(point3 - point1);

   normal = edge1.cross(edge2);

   m_normals->at(three_v1) += normal[0];
   m_normals->at(three_v1 + 1) += normal[1];
   m_normals->at(three_v1 + 2) += normal[2];

   m_normals->at(three_v2) += normal[0];
   m_normals->at(three_v2 + 1) += normal[1];
   m_normals->at(three_v2 + 2) += normal[2];

   m_normals->at(three_v3) += normal[0];
   m_normals->at(three_v3 + 1) += normal[1];
   m_normals->at(three_v3 + 2) += normal[2];
  }

  normaliseNormals();
}

void Mesh::normaliseNormals()
{
    unsigned int three_i;
    for(unsigned int i = 0; i < m_vertCount; i++)
    {
        float magnitude = 0.0;
        three_i = 3 * i;
        float x = m_normals->at(three_i);
        float y = m_normals->at(three_i + 1);
        float z = m_normals->at(three_i + 2);
        magnitude = sqrt(x * x + y * y + z * z);

        if(abs(magnitude) > 1.0)
        {
         m_normals->at(three_i) /= magnitude;
         m_normals->at(three_i + 1) /= magnitude;
         m_normals->at(three_i + 2) /= magnitude;
        }
    }
}

void Mesh::buildVertexNormalVector()
 {
     if(m_vertNormalLines->size()<1)
     {
         for(unsigned int k=0; k < m_vertCount * 6; ++k)
         {
           m_vertNormalLines->push_back(0.0);
         }
     }
     else
     {
       for(unsigned int k=0; k<m_vertNormalLines->size(); ++k)
       {
         m_vertNormalLines->at(k) = 0.0;
       }
     }

     unsigned int three_i;
     float alpha = 1.01;
     float beta = 0.5;

     for (unsigned int i=0; i < m_vertCount; ++i)
     {
         three_i = 3*i;
         m_vertNormalLines->at(three_i) = alpha * m_vertices->at(three_i);
         m_vertNormalLines->at(three_i + 1) = alpha * m_vertices->at(three_i + 1);
         m_vertNormalLines->at(three_i + 2) = alpha * m_vertices->at(three_i + 2);
         m_vertNormalLines->at(three_i + 3) = m_vertices->at(three_i) + beta * m_normals->at(three_i);
         m_vertNormalLines->at(three_i + 4) = m_vertices->at(three_i + 1) + beta * m_normals->at(three_i + 1);
         m_vertNormalLines->at(three_i + 5) = m_vertices->at(three_i + 2) + beta * m_normals->at(three_i + 2);
     }
 }

void Mesh::normaliseMesh()
{
 //Find min and max for x, y, z
 float min_x = 1000.0;
 float min_y = 1000.0;
 float min_z = 1000.0;
 float max_x = -1000.0;
 float max_y = -1000.0;
 float max_z = -1000.0;
 float aux;
 unsigned int three_i;

 for (unsigned int i = 0; i < m_vertCount; ++i)
  {
        three_i = 3*i;

        aux = m_vertices->at(three_i);
        if(aux < min_x) { min_x = aux;}
        else if (aux > max_x) { max_x = aux;}

        aux = m_vertices->at(three_i + 1);
        if(aux < min_y) { min_y = aux;}
        else if (aux > max_y) { max_y = aux;}

        aux = m_vertices->at(three_i + 2);
        if(aux < min_z) { min_z = aux;}
        else if (aux > max_z) { max_z = aux;}
    }

    for (unsigned int i = 0; i < m_vertCount; ++i)
    {
        three_i = 3*i;
        float x  = m_vertices->at(three_i);
        float y = m_vertices->at(three_i + 1);
        float z = m_vertices->at(three_i + 2) ;

        m_vertices->at(three_i) = 2.0 * (x - min_x) / (max_x - min_x) - 1.0;
        m_vertices->at(three_i + 1) = 2.0 * (y - min_y) / (max_y - min_y) - 1.0;
        m_vertices->at(three_i + 2) = 2.0 * (z - min_z) / (max_z - min_z) - 1.0;
    }
}

void Mesh::bindVAOs()
{
    //VAO
    glGenVertexArrays(1, &m_vao);
    glBindVertexArray(m_vao);

 //Bind those vertex positions
  if(m_vertCount > 0)
    {
     glGenBuffers(1, &m_vboPositions);
     glBindBuffer(GL_ARRAY_BUFFER, m_vboPositions);
     glBufferData(GL_ARRAY_BUFFER, 3 * m_vertCount * sizeof(GLfloat), &m_vertices->at(0), GL_STATIC_DRAW);
     glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, NULL); //0 corresponds to vertex positions in VAO
     glEnableVertexAttribArray(0);
    }
  else
    {
     printf("ERROR: No vertices! \n");
     return;
    }

 //Bind those normals positions
  if(m_normals->size() > 0)
    {
     glGenBuffers(1, &m_vboNormals);
     glBindBuffer(GL_ARRAY_BUFFER, m_vboNormals);
     glBufferData(GL_ARRAY_BUFFER, 3 * m_vertCount  * sizeof(GLfloat), &m_normals->at(0), GL_STATIC_DRAW);
     glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 0, NULL); //1 corresponds to normals in VAO
     glEnableVertexAttribArray(1);
    }
  else
    {
      printf("ERROR: No normals! \n");
      return;
    }

   if(m_vertCount > 0)
    {
      if(!m_segmentationMode)
      {
       glGenBuffers(1, &m_vboIndices);
       glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIndices);
       glBufferData(GL_ELEMENT_ARRAY_BUFFER, 3 * m_faceCount * sizeof(GLuint), &m_faceIndices->at(0), GL_STATIC_DRAW);
      }
      else
      {
       glGenBuffers(1, &m_vboIndices);
       glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_vboIndices);
       glBufferData(GL_ELEMENT_ARRAY_BUFFER, m_segmentsIndices->size() * sizeof(GLuint), &m_segmentsIndices->at(0), GL_STATIC_DRAW);
      }
    }
   else
   {
      printf("ERROR: No vertices! \n");
      return;
   }  
}

void Mesh::unbindVAOs()
{
 glDeleteBuffers(1, &m_vboPositions);
 glDeleteBuffers(1, &m_vboIndices);
 glDeleteBuffers(1, &m_vboNormals);
 glDeleteVertexArrays(1, &m_vao);
}

void Mesh::buildArcNodeMatrix()
{
  //Create the arc-node adjacency matrix

    if(!m_adjMat)
    {
     m_adjMat = new std::map<std::pair<unsigned int, unsigned int>, short >();
     std::map <std::pair<unsigned int, unsigned int>, short >::iterator it;
     unsigned int three_i;
     unsigned int v1;
     unsigned int v2;
     unsigned int v3;
     unsigned int min;
     unsigned int max;

     for (unsigned int i = 0; i < m_faceCount; ++i)
     {
        three_i = 3*i;
        v1 = m_faceIndices->at(three_i);
        v2 = m_faceIndices->at(three_i + 1);
        v3 = m_faceIndices->at(three_i + 2);

        min = v1 < v2? v1 : v2;
        max = v1 > v2? v1 : v2;
        it = m_adjMat->find(make_pair(min, max));
        if(it == m_adjMat->end())
        {
            m_adjMat->insert(make_pair(std::pair<unsigned int, unsigned int>(min, max), 0));
            m_edgeCount++;
        }

        min = v2 < v3? v2 : v3;
        max = v2 > v3? v2 : v3;
        it = m_adjMat->find(make_pair(min, max));
        if(it == m_adjMat->end())
        {
            m_adjMat->insert(make_pair(std::pair<unsigned int, unsigned int>(min, max), 0));
            m_edgeCount++;
        }

        min = v1 < v3 ? v1 : v3;
        max = v1 > v3 ? v1 : v3;
        it = m_adjMat->find(make_pair(min, max));
        if(it == m_adjMat->end())
        {
            m_adjMat->insert(make_pair(std::pair<unsigned int, unsigned int>(min, max), 0));
            m_edgeCount++;
        }
     }
   }
}

void Mesh::buildVertexMatrix()
{
  if(!m_D)
  {
     m_D = new SparseMatrix<float>(m_vertCount, 4 * m_vertCount);
     m_D->reserve(m_vertCount * 4);
  }

  unsigned int three_i = 0;
  unsigned int four_i = 0;

  for(unsigned int i = 0; i < m_vertCount; i++)
  {
   three_i = 3 * i;
   four_i = 4 * i;
   m_D->insert(i, four_i) = m_vertices->at(three_i);
   m_D->insert(i, four_i + 1) =  m_vertices->at(three_i + 1);
   m_D->insert(i, four_i + 2) = m_vertices->at(three_i + 2);
   m_D->insert(i, four_i + 3) = 1;
  }

  m_D->makeCompressed();
}

void Mesh::buildNeighbourList()
{
 if(m_adjMat)
 {
  int v1, v2;

 //if(m_neighbours)
 // {
  //  delete [] m_neighbours;
 // }
  m_neighbours = new std::vector<std::vector<int> >(m_vertCount);


  for(std::map< std::pair<unsigned int, unsigned int>, short>::iterator it = m_adjMat->begin(); it != m_adjMat->end(); ++it)
  {
      v1 = it->first.first;
      v2 = it->first.second;

      m_neighbours->at(v1).push_back(v2);
      m_neighbours->at(v2).push_back(v1);
  }
 }
 else
 {
  buildArcNodeMatrix();
  buildNeighbourList();
 }
}

Vector3f Mesh::getNormal(unsigned int _vertNo)
{
    Vector3f normal(0.0, 0.0, 0.0);

    if(m_vertCount > _vertNo)
    {
        normal[0] = m_normals->at(_vertNo * 3);
        normal[1] = m_normals->at(_vertNo * 3 + 1);
        normal[2] = m_normals->at(_vertNo * 3 + 2);
    }
    return normal;
}

Vector3f Mesh::getVertex(unsigned int _vertNo)
{
    Vector3f vert(0.0, 0.0, 0.0);

    if(m_vertCount > _vertNo)
    {
     vert[0] = m_vertices->at(_vertNo * 3);
     vert[1] = m_vertices->at(_vertNo * 3 + 1);
     vert[2] = m_vertices->at(_vertNo * 3 + 2);
    }
    return vert;
}

bool Mesh::findInListOfNeighbours(int _neighbour1, int _neighbour2)
{
  std::vector<int> neighbours = m_neighbours->at(_neighbour1);
  unsigned int size = neighbours.size();
  bool found = false;

  for(unsigned int i=0; i<size && !found; ++i)
  {
      if(neighbours.at(i) == _neighbour2)
      {
          found = true;
      }
  }

  return found;
}

float Mesh::calculateVertexCurvature(int _index)
{

    float curvature = -10000;
    std::vector<int> neighbours = m_neighbours->at(_index);
    unsigned int noNeighbours = neighbours.size();
    std::vector<int> edges;
    int neighbour1;
    int neighbour2;
    float angleSum = 0.0;
    float areaSum = 0.0;
    float diff = 0.0;
    float dirac = 0.0;

    //Create triangle list
    for(unsigned int i = 0; i < noNeighbours - 1; ++i)
    {
      for(unsigned int j = i+1; j < noNeighbours; ++j)
      {
        neighbour1 = neighbours.at(i);
        neighbour2 = neighbours.at(j);

        if(findInListOfNeighbours(neighbour1, neighbour2))
        {
         //Found neighbour2 in list of neighbours of neighbour1
          edges.push_back(neighbour1);
          edges.push_back(neighbour2);
        }
      }
    }

      //we know the 3 indices of the traingle
      //calculate index angle and triangle area
      Vector3f v1 = getVertex(_index);
      unsigned int i = 0;

      while(i < edges.size())
       {
           Vector3f v2 = getVertex(edges.at(i));
           Vector3f v3 = getVertex(edges.at(i+1));

           //Dirac delta
           diff = euclideanDistance(v1, v2);
           dirac = 1/diff; //inversely proportional
           //Law of cosines
           float a = euclideanDistance(v1, v2);
           float b = euclideanDistance(v1, v3);
           float c = euclideanDistance(v2, v3);
           float s = (a + b + c)/2;
           float cosine = (a*a + b*b - c*c)/(2*a*b);
           float angle = acos(cosine);
           angleSum += angle;
           areaSum += sqrt(s*(s-a)*(s-b)*(s-c));
           i = i + 2;
        }

     curvature = (3 * (2 * 3.14159 - angleSum) * dirac * dirac) / areaSum;

     return curvature;
}

int Mesh::whereIsIntersectingMesh(bool _culling, int _originTemplateIndex, Vector3f _origin, Vector3f _ray)
{
 ///@ref Fast, Minimum Storage Ray/Triangle Intersection, Möller & Trumbore. Journal of Graphics Tools, 1997.
    Vector3f point1, point2, point3;
    Vector3f edge1, edge2;
    Vector3f P, Q, T;
    float det, inv_det, u, v;
    float t = 0.0f;
    unsigned int i = 0;
    unsigned int three_i;
    int v1, v2, v3;


    while(i < m_faceCount)
    {
      three_i = 3*i;
      i++;
      v1 = m_faceIndices->at(three_i);
      v2 = m_faceIndices->at(three_i+ 1);
      v3 = m_faceIndices->at(three_i + 2);

      if(v1 != _originTemplateIndex && v2 != _originTemplateIndex && v3 != _originTemplateIndex)
      {
        point1 = getVertex(v1);
        point2 = getVertex(v2);
        point3 = getVertex(v3);

        //Find vectors for two edges sharing V1
        edge1 = point2 - point1;
        edge2 = point3 - point1;

        //Begin calculating determinant - also used to calculate u parameter
         P = _ray.cross(edge2);

        //if determinant is near zero, ray lies in plane of triangle
         det = edge1.dot(P);


         if(_culling)
         {
           //Define test_cull if culling is desired
           if(det < EPSILON) continue;

           //calculate distance from origin to point1
           T = _origin - point1;

           //Calculate u parameter and test bound
           u = T.dot(P);

           //The intersection lies outside of the triangle
           if(u < 0.f || u > det) continue;

           //Prepare to test v parameter
           Q = T.cross(edge1);

           //Calculate V parameter and test bound
           v = _ray.dot(Q);

           //The intersection lies outside of the triangle
           if(v < 0.f || u + v  > det) continue;

           t = edge2.dot(Q);
           inv_det = 1.0f/det;

           //Scale parameters
           t *= inv_det;
           u *= inv_det;
           v *= inv_det;
         }

         else //Non-culling
         {
            if(det > -EPSILON && det < EPSILON) continue;
            inv_det = 1.0f/det;

            //calculate distance from origin to point1
            T = _origin - point1;

            //Calculate u parameter and test bound
            u = T.dot(P) * inv_det;

            //The intersection lies outside of the triangle
            if(u < 0.f || u > 1.0f) continue;

            //Prepare to test v parameter
            Q = T.cross(edge1);

            //Calculate V parameter and test bound
            v = _ray.dot(Q) * inv_det;

            //The intersection lies outside of the triangle
            if(v < 0.f || u + v  > 1.0f) continue;

            t = edge2.dot(Q) * inv_det;
         }

        if(t > EPSILON)
        { //ray intersection
           //Find intersection point
            Vector3f intersection = _origin + t * _ray;
            float min = 100;
            int imin;

            float distance1 = euclideanDistance(intersection, point1);
            float distance2 = euclideanDistance(intersection, point2);
            float distance3 = euclideanDistance(intersection, point3);

            if(distance1 < min)
            {
              min = distance1;
              imin = v1;
            }

            if (distance2 < min)
            {
                min = distance2;
                imin = v2;
            }

            if(distance3 < min)
            {
               min = distance3;
               imin = v3;
            }

            return imin;
        }
      }
    }

 return -1;
}

void Mesh::affineTransformation(MatrixXf _X)
{
    unsigned int three_i;
    Vector3f vertex;

    for(unsigned int i=0; i < m_vertCount; ++i)
    {
        three_i = 3*i;
        vertex[0] = m_vertices->at(three_i);
        vertex[1] = m_vertices->at(three_i + 1);
        vertex[2] = m_vertices->at(three_i + 2);

        vertex[0] = _X(0,0) * vertex[0] + _X(1, 0)* vertex[1] + _X(2, 0)*vertex[2] + _X(3, 0);
        vertex[1] = _X(0,1) * vertex[0] + _X(1, 1)* vertex[1] + _X(2, 1)*vertex[2] + _X(3, 1);
        vertex[2] = _X(0,2) * vertex[0] + _X(1, 2)* vertex[1] + _X(2, 2)*vertex[2] + _X(3, 2);

        m_vertices->at(three_i) = vertex[0];
        m_vertices->at(three_i + 1) = vertex[1];
        m_vertices->at(three_i + 2) = vertex[2];
    }

}

float Mesh::euclideanDistance(Vector3f _v1, Vector3f _v2)
{
 float diff1 = _v1[0] - _v2[0];
 float diff2 = _v1[1] - _v2[1];
 float diff3 = _v1[2] - _v2[2];

 return sqrt(diff1 * diff1 + diff2 * diff2 + diff3 * diff3);
}

void Mesh::moveObject(float _tX, float _tY, float _tZ)
{
  int three_i;
  mat4 T = translate(identity_mat4(), vec3(_tX, _tY, _tZ));
  vec4 vertex;
  vertex.v[3] = 1.0;

  for(unsigned int i=0; i<m_vertCount; ++i)
  {
     three_i = 3*i;
     vertex.v[0] = m_vertices->at(three_i);
     vertex.v[1] = m_vertices->at(three_i + 1);
     vertex.v[2] = m_vertices->at(three_i + 2);

     vertex = T * vertex;

     m_vertices->at(three_i) = vertex.v[0];
     m_vertices->at(three_i + 1) = vertex.v[1];
     m_vertices->at(three_i + 2) = vertex.v[2];
  }

  calculatePosition();
}

void Mesh::moveObject(Vector3f _trans)
{
  moveObject(_trans[0], _trans[1], _trans[2]);
}

void Mesh::moveToCentre()
{
   moveObject(-m_position[0], -m_position[1], -m_position[2]);
}

void Mesh::rotateObject(float _angleX, float _angleY, float _angleZ)
{
  unsigned int three_i;
  mat4 Rx = rotate_x_deg(identity_mat4(), _angleX);
  mat4 Ry= rotate_y_deg(identity_mat4(), _angleY);
  mat4 Rz = rotate_z_deg(identity_mat4(), _angleZ);
  mat4 R = Rz * Ry * Rx;
  vec4 vertex;
  vertex.v[3] = 1.0;

  for(unsigned int i=0; i<m_vertCount; ++i)
  {
     three_i = 3*i;
     vertex.v[0] = m_vertices->at(three_i);
     vertex.v[1] = m_vertices->at(three_i + 1);
     vertex.v[2] = m_vertices->at(three_i + 2);

     vertex = R * vertex;

     m_vertices->at(three_i) = vertex.v[0];
     m_vertices->at(three_i + 1) = vertex.v[1];
     m_vertices->at(three_i + 2) = vertex.v[2];
  }

  calculatePosition();
}

void Mesh::rotateObject(Matrix3f _R)
{
    unsigned int three_i;
    Vector3f vertex;

    for(unsigned int i=0; i<m_vertCount; ++i)
    {
       three_i = 3*i;
       vertex[0] = m_vertices->at(three_i);
       vertex[1] = m_vertices->at(three_i + 1);
       vertex[2] = m_vertices->at(three_i + 2);

       vertex = _R * vertex;

       m_vertices->at(three_i) = vertex[0];
       m_vertices->at(three_i + 1) = vertex[1];
       m_vertices->at(three_i + 2) = vertex[2];
    }

    calculatePosition();
}

bool Mesh::areEigenvectorsOrthogonal()
{
    Vector3f v1;
    Vector3f v2;
    Vector3f v3;

    v1[0] = m_eigenvectors(0, 0);
    v1[1] = m_eigenvectors(1, 0);
    v1[2] = m_eigenvectors(2, 0);

    v2[0] = m_eigenvectors(0, 1);
    v2[1] = m_eigenvectors(1, 1);
    v2[2] = m_eigenvectors(2, 1);

    v3[0] = m_eigenvectors(0, 2);
    v3[1] = m_eigenvectors(1, 2);
    v3[2] = m_eigenvectors(2, 2);

    double dot12 = v1[0]*v2[0] + v1[1]*v2[1] + v1[2]*v2[2];
    double dot23 = v2[0]*v3[0] + v2[1]*v3[1] + v2[2]*v3[2];

    bool isDot12 = (dot12 < 0.1)&&(dot12 > -0.1);
    bool isDot23 = (dot23 < 0.1)&&(dot23 > -0.1);

    if(isDot12 && isDot23)
    {
      return true;
    }

    return false;
}

void Mesh::rotateByEigenVectors()
{
    //after moving to centre of OpenGL screen

  if (areEigenvectorsOrthogonal())
  {
      Matrix3f A = m_eigenvectors;
      Matrix3f B;
      B.setZero();

      for(unsigned int k=0; k<3; ++k)
      {
        for(unsigned int l=0; l<3; ++l)
        {
            if(abs(A(k,l)) > 0.8)
            {
              B(k,l) = 1.0;
            }
        }
      }


      Matrix3f R = B*A.inverse();
      Vector3f vertex;
      int three_i;

      for(unsigned int i=0; i<m_vertCount; ++i)
      {
        three_i = 3*i;

        vertex[0] = m_vertices->at(three_i);
        vertex[1] = m_vertices->at(three_i + 1);
        vertex[2] = m_vertices->at(three_i + 2);

        vertex = R * vertex;

        m_vertices->at(three_i) = vertex[0];
        m_vertices->at(three_i + 1) = vertex[1];
        m_vertices->at(three_i + 2) = vertex[2];
      }

      calculatePosition();
   }
  else
  {
      //Eigenvectors are not orthonormal
      //Build new ones?
  }
}

void Mesh::calculateEigenvectors()
{
    Matrix3f covarianceMatrix;
    covarianceMatrix.setZero();
    unsigned int three_i;
    Vector3f v;

    //Covariance Matrix
    for(unsigned int i=0; i<m_vertCount; ++i)
    {
       three_i = 3*i;
       v[0] = m_vertices->at(three_i) - m_position[0];
       v[1] = m_vertices->at(three_i+1) - m_position[1];
       v[2] = m_vertices->at(three_i+2) - m_position[2];

       for(unsigned int j=0; j<3; ++j)
       {
         for(unsigned int k=0; k<3; ++k)
         {
           covarianceMatrix(j, k) = covarianceMatrix(j, k) + v[j]*v[k];
         }
       }
    }
    covarianceMatrix /= (m_vertCount-1);

    SelfAdjointEigenSolver<Matrix3f> es;
    es.compute(covarianceMatrix);

    //Eigenvectors and eigenvalues
    m_eigenvectors = es.eigenvectors();
    m_eigenvalues = es.eigenvalues();
}

void Mesh::calculatePosition()
{
    unsigned int three_i;
    m_position[0] = 0.0;
    m_position[1] = 0.0;
    m_position[2] = 0.0;

    for(unsigned int i=0; i<m_vertCount; ++i)
    {
        three_i = 3*i;

        m_position[0] += m_vertices->at(three_i);
        m_position[1] += m_vertices->at(three_i + 1);
        m_position[2] += m_vertices->at(three_i + 2);
    }

    m_position[0] /= m_vertCount;
    m_position[1] /= m_vertCount;
    m_position[2] /= m_vertCount;
}


void Mesh::segmentMesh()
{
    unsigned int size = m_landmarkVertexIndices->size();

    if(size >= 3)
    {
        Vector3i plane;
        Vector3f normal;
        Vector3f point1;
        Vector3f point2;
        Vector3f point3;

        for(unsigned int i=0; i<3; ++i)
        {
          plane[i] = m_landmarkVertexIndices->at(size-1);
          m_landmarkVertexIndices->pop_back();
          size--;
        }

        for(unsigned int i=0; i<3; ++i)
        {
          point1[i] = m_vertices->at(3*plane[0] + i);
          point2[i] = m_vertices->at(3*plane[1] + i);
          point3[i] = m_vertices->at(3*plane[2] + i);
        }

        normal = (point2 - point1).cross(point3 - point1);
        normal.normalize();

        m_segmentationRoot = segmentationProcedure(plane, normal, m_segmentationRoot, m_segmentationRoot);
        createSegmentList();
        m_segmentationMode = true;
    }
    else
    {
      //Do nothing
    }
}

Segmentation* Mesh::segmentationProcedure(Vector3i _plane, Vector3f _normal, Segmentation* _segment, Segmentation* _parent)
{

    if(_segment)
    {
       _segment->m_leftSegment = segmentationProcedure(_plane, _normal, _segment->m_leftSegment, _segment);
       _segment->m_rightSegment = segmentationProcedure(_plane, _normal, _segment->m_rightSegment, _segment);
    }

    else
    {
     //Segment is null, so it will contain plane and new segments

        Vector3i face;
        Vector3f faceNormal;
        Vector3f point1;
        Vector3f point2;
        Vector3f point3;
        Vector3f planeCentre = calculateCentre(_plane[0], _plane[1], _plane[2]);
        Vector3f faceCentre;
        Vector3f planeFaceDirection;
        unsigned int size;

        _segment = new Segmentation();
        _segment->m_plane = _plane;
        _segment->m_visited = false;
        _segment->m_leftFaces = new std::vector<GLuint>();
        _segment->m_rightFaces = new std::vector<GLuint>();

      if(_parent)
        {
        //Parent exists
        //Left side
            size = _parent->m_leftFaces->size();

            for(unsigned int i=0; i<size/3; ++i)
            {
              face[0] = _parent->m_leftFaces->at(3*i);
              face[1] = _parent->m_leftFaces->at(3*i + 1);
              face[2] = _parent->m_leftFaces->at(3*i + 2);

              faceCentre = calculateCentre(face[0], face[1], face[2]);

             for(unsigned int j=0; j<3; ++j)
             {
               point1[j] = m_vertices->at(3*face[0] + j);
               point2[j] = m_vertices->at(3*face[1] + j);
               point3[j] = m_vertices->at(3*face[2] + j);
             }

             faceNormal = (point2 - point1).cross(point3 - point1);
             faceNormal.normalize();
             planeFaceDirection = faceCentre - planeCentre;
             planeFaceDirection.normalize();

             float dot_normals = _normal.dot(faceNormal);
             float dot_directions = _normal.dot(planeFaceDirection);

             if(dot_directions >= 0 && dot_directions <= 1)
             {
               //Above plane - segment left
               _segment->m_leftFaces->push_back(face[0]);
               _segment->m_leftFaces->push_back(face[1]);
               _segment->m_leftFaces->push_back(face[2]);
             }
             else
             {
               //Below plane - segment right
               _segment->m_rightFaces->push_back(face[0]);
               _segment->m_rightFaces->push_back(face[1]);
               _segment->m_rightFaces->push_back(face[2]);
             }
            }


            //Right side
             size = _parent->m_rightFaces->size();

            for(unsigned int i=0; i<size/3; ++i)
            {
              face[0] = _parent->m_rightFaces->at(3*i);
              face[1] = _parent->m_rightFaces->at(3*i + 1);
              face[2] = _parent->m_rightFaces->at(3*i + 2);

             for(unsigned int j=0; j<3; ++j)
             {
               point1[j] = m_vertices->at(3*face[0] + j);
               point2[j] = m_vertices->at(3*face[1] + j);
               point3[j] = m_vertices->at(3*face[2] + j);
             }

             faceNormal = (point2 - point1).cross(point3 - point1);
             faceNormal.normalize();
             planeFaceDirection = faceCentre - planeCentre;
             planeFaceDirection.normalize();

             float dot_normals = _normal.dot(faceNormal);
             float dot_directions = _normal.dot(planeFaceDirection);

             if(dot_directions >= 0 && dot_directions <= 1)
             {
               //Above plane - segment left
               _segment->m_leftFaces->push_back(face[0]);
               _segment->m_leftFaces->push_back(face[1]);
               _segment->m_leftFaces->push_back(face[2]);
             }
             else
             {
               //Below plane - segment right
               _segment->m_rightFaces->push_back(face[0]);
               _segment->m_rightFaces->push_back(face[1]);
               _segment->m_rightFaces->push_back(face[2]);
             }
            }

            //Clear parent
            _parent->m_leftFaces->clear();
            _parent->m_rightFaces->clear();
         }
      else
      {
          size = m_faceIndices->size();

          for(unsigned int i=0; i<m_faceCount; ++i)
          {
            face[0] = m_faceIndices->at(3*i);
            face[1] = m_faceIndices->at(3*i + 1);
            face[2] = m_faceIndices->at(3*i + 2);

           for(unsigned int j=0; j<3; ++j)
           {
             point1[j] = m_vertices->at(3*face[0] + j);
             point2[j] = m_vertices->at(3*face[1] + j);
             point3[j] = m_vertices->at(3*face[2] + j);
           }

           faceNormal = (point2 - point1).cross(point3 - point1);
           faceNormal.normalize();

           float dot = _normal.dot(faceNormal);

           if(dot >= 0.0 && dot <= 1)
           {
             //Above plane - segment left
             _segment->m_leftFaces->push_back(face[0]);
             _segment->m_leftFaces->push_back(face[1]);
             _segment->m_leftFaces->push_back(face[2]);
           }
           else
           {
             //Below plane - segment right
             _segment->m_rightFaces->push_back(face[0]);
             _segment->m_rightFaces->push_back(face[1]);
             _segment->m_rightFaces->push_back(face[2]);
           }
          }
       }
    }

    return _segment;
}

void Mesh::createSegmentList()
{
  m_segments.clear();
  m_segmentsIndices->clear();


  std::vector<Segmentation*> stack;
  Segmentation* current;
  stack.push_back(m_segmentationRoot);


  while(stack.size() > 0)
  {
      current = stack.back();

      if(current->m_leftSegment != NULL && current->m_rightSegment != NULL)
      {
        if(!current->m_visited)
        {
          current->m_visited = true;
          stack.push_back(current->m_leftSegment);
          stack.push_back(current->m_rightSegment);
        }
        else
        {
          stack.pop_back();
        }
      }
      else
      {
          m_segments.push_back(current->m_leftFaces);
          m_segments.push_back(current->m_rightFaces);
          stack.pop_back();
      }
  }

  for(unsigned int i=0; i<m_segments.size(); ++i)
  {
      std::vector<GLuint>* segment = m_segments[i];
      for(unsigned int j=0; j<segment->size(); ++j)
      {
        m_segmentsIndices->push_back(segment->at(j));
      }
  }
}


void Mesh::destroySegments(Segmentation* _segmentation)
{
  if(_segmentation)
    {

      if(!_segmentation->m_leftSegment && !_segmentation->m_rightSegment)
            {
              delete [] _segmentation->m_leftFaces;
              delete [] _segmentation->m_rightFaces;
              delete _segmentation;
            }

            if (_segmentation->m_leftSegment)
            {
               destroySegments(_segmentation->m_leftSegment);
            }

            if (_segmentation->m_rightSegment)
            {
               destroySegments(_segmentation->m_rightSegment);
            }

            m_segments.clear();
            delete [] m_segmentsIndices;
     }
}


Vector3f Mesh::calculateCentre(int _p1, int _p2, int _p3)
{
   Vector3f point1;
   Vector3f point2;
   Vector3f point3;

   for(unsigned int i=0; i<3; ++i)
   {
       point1[i] = m_vertices->at(3*_p1 + i);
       point2[i] = m_vertices->at(3*_p2 + i);
       point3[i] = m_vertices->at(3*_p3 + i);
   }

   return (point1 + point2 + point3)/3;
}




