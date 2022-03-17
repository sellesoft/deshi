void Render::DisplayRenderStats(){
    using namespace UI;
    RenderStats* rstats = Render::GetStats();
    BeginRow("renderstatsaligned", 2, 0, UIRowFlags_AutoSize);{
        RowSetupColumnAlignments({{0,0.5},{1,0.5}});
        Text("total triangles: "); Text(toStr(rstats->totalTriangles).str);
        Text("total vertices: ");  Text(toStr(rstats->totalVertices).str);
        Text("total indices: ");   Text(toStr(rstats->totalIndices).str);
        Text("drawn triangles: "); Text(toStr(rstats->drawnTriangles).str);
        Text("drawn indicies: ");  Text(toStr(rstats->drawnIndices).str);
        Text("render time: ");     Text(toStr(rstats->renderTimeMS).str);
    }EndRow();
}   