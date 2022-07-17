namespace sss {

template <typename Vertex> void Mesh<Vertex>::init(const std::vector<Vertex>& vertices) {
  release();

  glCreateVertexArrays(1, &m_VA);
  glCreateBuffers(1, &m_VB);

  m_numVertices = vertices.size();
  m_numIndices = 0;
  glNamedBufferData(m_VB, (GLsizeiptr)(vertices.size() * sizeof(Vertex)), vertices.data(),
                    GL_STATIC_DRAW);

  glVertexArrayVertexBuffer(m_VA, 0, m_VB, 0, sizeof(Vertex));
  Vertex::initBindings(m_VA);
}

template <typename Vertex>
void Mesh<Vertex>::init(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices) {
  release();

  glCreateVertexArrays(1, &m_VA);
  glCreateBuffers(1, &m_VB);
  glCreateBuffers(1, &m_IB);

  m_numVertices = vertices.size();
  m_numIndices = indices.size();
  glNamedBufferData(m_VB, (GLsizeiptr)(m_numVertices * sizeof(Vertex)), vertices.data(),
                    GL_STATIC_DRAW);
  glNamedBufferData(m_IB, (GLsizeiptr)(m_numIndices * sizeof(GLuint)), indices.data(),
                    GL_STATIC_DRAW);

  glVertexArrayVertexBuffer(m_VA, 0, m_VB, 0, sizeof(Vertex));
  glVertexArrayElementBuffer(m_VA, m_IB);

  Vertex::initBindings(m_VA);
}

template <typename Vertex> void Mesh<Vertex>::release() {
  if (m_VA) {
    Vertex::cleanupBindings(m_VA);
    glDeleteVertexArrays(1, &m_VA);
  }

  if (m_VB)
    glDeleteBuffers(1, &m_VB);
  if (m_IB)
    glDeleteBuffers(1, &m_IB);
}

template <typename Vertex> void Mesh<Vertex>::render(const ShaderProgram& program) const {
  program.use();
  glBindVertexArray(m_VA);

  if (m_numIndices)
    glDrawElements(GL_TRIANGLES, m_numIndices, GL_UNSIGNED_INT, nullptr);
  else
    glDrawArrays(GL_TRIANGLES, 0, m_numVertices);

  glBindVertexArray(0);
}

} // namespace sss
