#include "Canvas.h"
#include "Main.h"

wxBEGIN_EVENT_TABLE(Canvas, wxPanel)
	EVT_PAINT(Canvas::OnPaint)
	EVT_SIZE(Canvas::Resized)
    EVT_MOUSEWHEEL(Canvas::OnScroll)
    EVT_MOTION(Canvas::OnMouseMove)
wxEND_EVENT_TABLE()

Canvas::Canvas(wxWindow* parent, const wxGLAttributes& attribs)
    : wxGLCanvas(parent, attribs)
{
	SetBackgroundStyle(wxBG_STYLE_CUSTOM);
	mainPtr = (Main*)parent;

    // === GL Initialization ===
    wxGLContextAttrs ctxAttrs;
    ctxAttrs.PlatformDefaults().OGLVersion(4, 6).ResetIsolation();
#ifdef _DEBUG
    ctxAttrs.DebugCtx();
#else
    ctxAttrs.Robust();
#endif
    ctxAttrs.EndList();
    context = new wxGLContext(this, nullptr, &ctxAttrs);

    while (!IsShown()) {};
    SetCurrent(*context);

    glLoadIdentity();
    GLenum err = glewInit();
    if (GLEW_OK != err)
    {
        std::cerr << "Glew initialization failed.\n";
        exit(-1);
    }
    wglSwapIntervalEXT(0);

    // Blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // === Rendering Setup ===
    shader = Shader::CompileString(
#include "basicshader"
    );

    va = new VertexArray;
    vb = new VertexBuffer;
    vbl = new VertexBufferLayout;

    vbl->Push<float>(2);
    va->AddVBuffer(*vb, *vbl);

    // Initialize text renderer
    textRenderer = new TextRenderer;

    shader->Bind();
    va->Bind();

    // Initialize function renderer object
    renderer = new RendererType([&]() { this->JobProcessingFinished(); });
}

Canvas::~Canvas()
{
    delete context;
    delete vb;
    delete vbl;
    delete va;
    delete shader;
    delete textRenderer;

    delete renderer;
}

void Canvas::ResetView()
{
    relXScale = 300.0, relYScale = 300.0;
    xOffset = 0.0, yOffset = 0.0;

    UpdateJobs();
}

void Canvas::JobProcessingFinished()
{
    Update();
    Refresh();
}

void Canvas::DisplayStandard(bool display)
{
    displayStandard = display;
    renderer->UpdateJobs();
}

void Canvas::DisplaySeeds(bool display)
{
    displaySeeds = display;
    renderer->KeepSeeds(display);
}

void Canvas::DisplayMeshes(bool display)
{
    displayMeshes = display;
    renderer->KeepMesh(display);
}

void Canvas::OnDraw()
{
    glViewport(0, 0, w, h);

    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    // === Drawing code ===
    // Determine Grid Spacing
    auto [mantissa, exponent] = DetermineGridSpacing();
    double gridW = pow(10, exponent) * mantissa;

    // Gridlines
    DrawGridlines(gridW, 0.3f); // Major
    DrawGridlines(gridW / 5, 0.1f); // Minor

    // Equations
    for (std::shared_ptr<Job> job : renderer->jobs)
    {
        if (displaySeeds)
        {
            // Draw seeds
            auto seeds = renderer->GetSeeds(job->id);
            if (seeds.has_value())
                DrawSeeds(seeds.value());
        }

        if (displayMeshes)
        {
            // Draw filtering meshes
            auto mesh = renderer->GetMesh(job->id);
            if (mesh.has_value())
                DrawMesh(mesh.value());
        }

        if (displayStandard)
        {
            // Draw final contour

            std::lock_guard lock(job->bufferMutex);
            DrawContour(job->bufferedVerts, job->col);
        }
    }

    // Axes
    DrawAxes(2.0f);

    // Axes text
    DrawAxisText({ mantissa, exponent });
    shader->Bind();
    va->Bind();

    SwapBuffers();
}

void Canvas::OnPaint(wxPaintEvent& evt)
{
    RecalculateBounds();

    OnDraw();
	evt.Skip();
}

void Canvas::Resized(wxSizeEvent& evt)
{
    int newW, newH;
    GetSize(&newW, &newH);
    if (w)
    {
        xOffset *= (double)w / newW;
        yOffset *= (double)h / newH;
    }
    UpdateJobs();
    
	evt.Skip();
}

void Canvas::OnScroll(wxMouseEvent& evt)
{
    // Zoom
    double factor = 1.5;
    if (evt.GetWheelRotation() < 0) { factor = 1.0 / factor; }

    double mouseX = evt.GetX();
    double mouseY = evt.GetY();

    xOffset += (factor - 1) * (2 * mouseX / w + xOffset - 1);
    yOffset += (factor - 1) * (-2 * mouseY / h + yOffset + 1);

    relXScale *= factor;
    relYScale *= factor;

    UpdateJobs();
    Refresh();
    evt.Skip();
}

void Canvas::OnMouseMove(wxMouseEvent& evt)
{
    if (!evt.LeftIsDown())
    {
        lastMouse.valid = false;
    }
    else
    {
        if (lastMouse.valid)
            OnMouseDrag((double)evt.GetX() - lastMouse.x, (double)evt.GetY() - lastMouse.y);

        lastMouse.valid = true;
        lastMouse.x = evt.GetX();
        lastMouse.y = evt.GetY();
    }
    evt.Skip();
}

void Canvas::OnMouseDrag(double delX, double delY)
{
    xOffset -= delX / w * 2;
    yOffset += delY / h * 2;

    UpdateJobs();
    Refresh();
}

void Canvas::ToScreen(float& xout, float& yout, double x, double y)
{
    xout = (float)(x * relXScale / w - xOffset);
    yout = (float)(y * relYScale / h - yOffset);
}

void Canvas::DrawAxes(float width)
{
    float xzeroS = (float)(-xOffset);
    float yzeroS = (float)(-yOffset);

    float lineW = width / w;
    float lineH = width / h;

    Point p[8] = { { -1, yzeroS - lineH }, { -1, yzeroS + lineH },
                            { 1, yzeroS - lineH }, { 1, yzeroS + lineH },
                            { xzeroS - lineW, -1 }, { xzeroS + lineW, -1 },
                            { xzeroS - lineW, 1 }, { xzeroS + lineW, 1 } };
    std::array<Point, 12> axisBuf = { p[0], p[1], p[2], p[1], p[2], p[3],
        p[4], p[5], p[6], p[5], p[6], p[7] };

    vb->SetData(axisBuf.data(), axisBuf.size() * sizeof(Point));

    glUniform4f(shader->GetUniformLocation("col"), 0.0f, 0.0f, 0.0f, 1.0f);
    glDrawArrays(GL_TRIANGLES, 0, 12);
}

void Canvas::DrawGridlines(double spacing, float opacity)
{
    std::vector<Point> linesBuf;

    // Horizontal lines
    double startY = ceil(bounds.ymin / spacing) * spacing;
    int num = (int)floor(bounds.h() / spacing);

    for (int yi = 0; yi <= num; yi++)
    {
        double worldY = startY + spacing * yi;
        float screenY = (float)(worldY * relYScale / h - yOffset);

        linesBuf.push_back({ -1, screenY });
        linesBuf.push_back({ 1, screenY });
    }

    // Vertical lines
    double startX = ceil(bounds.xmin / spacing) * spacing;
    num = (int)floor(bounds.w() / spacing);

    for (int xi = 0; xi <= num; xi++)
    {
        double worldX = startX + spacing * xi;
        float screenX = (float)(worldX * relXScale / w - xOffset);

        linesBuf.push_back({ screenX, -1 });
        linesBuf.push_back({ screenX, 1 });
    }

    vb->SetData(linesBuf.data(), linesBuf.size() * sizeof(Point));

    glUniform4f(shader->GetUniformLocation("col"), 0.0f, 0.0f, 0.0f, opacity);
    glDrawArrays(GL_LINES, 0, (int)linesBuf.size());
}

std::pair<int, int> Canvas::DetermineGridSpacing()
{
    wxDisplay display((unsigned int)wxDisplay::GetFromWindow(this));
    wxRect screen = display.GetClientArea();

    double targetMajorSize = std::max(screen.width, screen.height) / 15.0;

    // Find grid width that achieves target
    double exactGridW = targetMajorSize / w * bounds.w();
    return RoundMajorGridValue(exactGridW);
}

std::pair<int, int> Canvas::RoundMajorGridValue(double val)
{
    int exponent = (int)floor(log(val) / log(10));

    std::pair<int, int> options[] = { { 5, exponent - 1 }, { 1, exponent }, { 2, exponent },
        { 5, exponent }, { 1, exponent + 1 } };

    auto bestOption = options[0];
    double bestDist = DBL_MAX;
    for (const auto& option : options)
    {
        double dist = std::abs(val - option.first * std::pow(10, option.second));
        if (dist < bestDist) { bestOption = option; bestDist = dist; }
    }

    return bestOption;
}

void Canvas::DrawAxisText(std::pair<int, int> spacingSF)
{
    double spacing = spacingSF.first * pow(10, spacingSF.second);

    float screenX, screenY;

    // y-axis text
    double startY = ceil(bounds.ymin / spacing) * spacing;
    int num = (int)floor(bounds.h() / spacing);
    screenX = (float)((1.0 - xOffset) / 2.0 * w + 4.0);
    screenX = std::max(screenX, 4.0f);
    screenX = std::min(screenX, w - 30.0f);
    for (int yi = 0; yi <= num; yi++)
    {
        double worldY = startY + spacing * yi;
        screenY = (float)((worldY * relYScale / h - yOffset + 1.0) / 2.0 * h);

        textRenderer->RenderText(std::format("{:.10}", worldY), { "Arial", 48 }, w, h, screenX, screenY, 0.5f);
    }

    // x-axis text
    double startX = ceil(bounds.xmin / spacing) * spacing;
    num = (int)floor(bounds.w() / spacing);
    screenY = (float)((1.0 - yOffset) / 2.0 * h + 4.0);
    screenY = std::max(screenY, 4.0f);
    screenY = std::min(screenY, h - 10.0f);
    for (int xi = 0; xi <= num; xi++)
    {
        double worldX = startX + spacing * xi;
        if ((int)ceil(bounds.xmin / spacing) + xi == 0) continue;
        screenX = (float)((worldX * relXScale / w - xOffset + 1.0) / 2.0 * w);

        textRenderer->RenderText(std::format("{:.10}", worldX), { "Arial", 48 }, w, h, screenX, screenY, 0.5f);
    }
}

void Canvas::DrawSeeds(const std::shared_ptr<Seeds>& seeds)
{
    std::vector<float> screenSeeds;
    
    // Count seeds
    size_t num = 0;
    for (const auto& seedVec : *seeds)
        num += seedVec.size();

    screenSeeds.reserve(num * 2);

    // Transform seeds to screen coordinates
    double xScale = relXScale / w;
    double yScale = relYScale / h;
    for (const auto& seedVec : *seeds)
    {
        for (const Seed& s : seedVec)
        {
            screenSeeds.push_back((float)(s.x * xScale - xOffset));
            screenSeeds.push_back((float)(s.y * yScale - yOffset));
        }
    }

    glUniform4f(shader->GetUniformLocation("col"), 0.0f, 0.0f, 0.0f, 1.0f);
    vb->SetData(screenSeeds.data(), screenSeeds.size() * sizeof(float));
    glDrawArrays(GL_POINTS, 0, (int)num);
}

void Canvas::DrawMesh(const std::shared_ptr<Mesh>& mesh)
{
    int dim = mesh->dim;
    std::vector<Point> verts;

    double boxW = mesh->bounds.w() / dim;
    double boxH = mesh->bounds.h() / dim;

    for (int by = 0; by < dim; by++)
    {
        for (int bx = 0; bx < dim; bx++)
        {
            if (!(*mesh).boxes[bx + by * dim]) continue;

            double x1 = mesh->bounds.xmin + boxW * bx;
            double x2 = x1 + boxW;

            double y1 = mesh->bounds.ymin + boxH * by;
            double y2 = y1 + boxH;

            // Transform point to screen coordinates
            float x1s, y1s, x2s, y2s;
            ToScreen(x1s, y1s, x1, y1);
            ToScreen(x2s, y2s, x2, y2);

            Point corners[4] = { { x1s, y1s }, { x1s, y2s }, { x2s, y2s }, { x2s, y1s } };

            verts.push_back(corners[0]);
            verts.push_back(corners[1]);
            verts.push_back(corners[2]);
            verts.push_back(corners[2]);
            verts.push_back(corners[3]);
            verts.push_back(corners[0]);
        }
    }

    glUniform4f(shader->GetUniformLocation("col"), 1.0f, 0.0f, 0.0f, 0.2f);
    vb->SetData(verts.data(), verts.size() * sizeof(Point));
    glDrawArrays(GL_TRIANGLES, 0, (int)verts.size());
}

void Canvas::DrawContour(const std::vector<double>& verts, const wxColour& col)
{
    if (verts.size() == 0) return;

    if (Arch::HasInstructions<AVX>())
    {
        TIMER(transform);
        // Transformation constants
        double xScale = relXScale / w;
        double yScale = relYScale / h;

        // Allocate output buffer, with same alignment as input buffer
        size_t offset = (uint64_t)verts.data() % 32;
        float* screenVerts = (float*)_aligned_offset_malloc(verts.size() * sizeof(float), 16, offset / 2);

        // Calculate position of packs
        uint64_t vecStart = (offset == 0) ? 0 : (32 - offset) / 8;
        __m256d* packs = (__m256d*) & verts[vecStart];
        __m128* outPacks = (__m128*) & screenVerts[vecStart];
        uint64_t packNum = (verts.size() - vecStart) / 4;

        // Compute first few transformations
        for (uint64_t i = 0; i < vecStart; i++)
        {
            if (i % 2 == 0) screenVerts[i] = (float)(verts[i] * xScale - xOffset);
            else screenVerts[i] = (float)(verts[i + 1] * yScale - yOffset);
        }

        // Packed transformations
        // Prepare packs for multiplication and subtraction
        __m256d mulPack;
        __m256d subPack;

        for (int i = 0; i < 4; i++)
        {
            if ((i + vecStart) % 2 == 0)
            {
                mulPack.m256d_f64[i] = xScale;
                subPack.m256d_f64[i] = xOffset;
            }
            else
            {
                mulPack.m256d_f64[i] = yScale;
                subPack.m256d_f64[i] = yOffset;
            }
        }

        // Complete transformations
        if (Arch::HasInstructions<FMA>())
        {
            for (uint64_t pi = 0; pi < packNum; pi++)
                outPacks[pi] = _mm256_cvtpd_ps(_mm256_fmsub_pd(packs[pi], mulPack, subPack));
        }
        else
        {
            for (uint64_t pi = 0; pi < packNum; pi++)
                outPacks[pi] = _mm256_cvtpd_ps(_mm256_sub_pd(_mm256_mul_pd(packs[pi], mulPack), subPack));
        }

        // Compute final few transformations
        for (uint64_t i = vecStart + packNum * 4; i < verts.size(); i++)
        {
            if (i % 2 == 0) screenVerts[i] = (float)(verts[i] * xScale - xOffset);
            else screenVerts[i] = (float)(verts[i + 1] * yScale - yOffset);
        }
        STOP_LOG(transform);

        vb->SetData(screenVerts, verts.size() * sizeof(float));
        glUniform4f(shader->GetUniformLocation("col"), col.Red() / 255.0f, col.Green() / 255.0f, col.Blue() / 255.0f, 1.0f);
        glDrawArrays(GL_LINES, 0, (int)verts.size() / 2);
        _aligned_free(screenVerts);
    }
    else
    {
        TIMER(transform);
        std::vector<float> screenVerts;
        screenVerts.reserve(verts.size());

        double xScale = relXScale / w;
        double yScale = relYScale / h;
        for (int i = 0; i < verts.size(); i += 2)
        {
            screenVerts.push_back((float)(verts[i] * xScale - xOffset));
            screenVerts.push_back((float)(verts[i + 1] * yScale - yOffset));
        }
        STOP_LOG(transform);

        vb->SetData(screenVerts.data(), screenVerts.size() * sizeof(float));
        glUniform4f(shader->GetUniformLocation("col"), col.Red() / 255.0f, col.Green() / 255.0f, col.Blue() / 255.0f, 1.0f);
        glDrawArrays(GL_LINES, 0, (int)screenVerts.size() / 2);
    }
}

void Canvas::RecalculateBounds()
{
    GetSize(&w, &h);

    bounds = { (double)w / relXScale * (xOffset - 1),
        (double)h / relYScale * (yOffset - 1),
        (double)w / relXScale * (xOffset + 1),
        (double)h / relYScale * (yOffset + 1) };
}

void Canvas::UpdateJobs()
{
    RecalculateBounds();
    for (std::shared_ptr<Job> job : renderer->jobs)
        job->bounds = bounds;

    renderer->UpdateJobs();
}