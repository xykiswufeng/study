#!/usr/bin/env python3
"""Convert a STEP/STP B-Rep model to the compact mesh used by wz-724."""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

from OCP.BRep import BRep_Tool
from OCP.BRepMesh import BRepMesh_IncrementalMesh
from OCP.IFSelect import IFSelect_RetDone
from OCP.STEPControl import STEPControl_Reader
from OCP.TopAbs import TopAbs_FACE, TopAbs_REVERSED, TopAbs_SOLID
from OCP.TopExp import TopExp_Explorer
from OCP.TopLoc import TopLoc_Location
from OCP.TopoDS import TopoDS


MAGIC = b"WZ3DMESH"
VERSION = 1
COLORS = (
    (47, 155, 202, 255),
    (231, 162, 45, 255),
    (83, 184, 153, 255),
    (151, 113, 207, 255),
    (208, 91, 104, 255),
    (104, 166, 220, 255),
    (198, 139, 92, 255),
    (111, 190, 214, 255),
)


def iter_shapes(shape, kind):
    explorer = TopExp_Explorer(shape, kind)
    while explorer.More():
        yield explorer.Current()
        explorer.Next()


def append_faces(owner, color, vertices, triangles):
    for raw_face in iter_shapes(owner, TopAbs_FACE):
        face = TopoDS.Face_s(raw_face)
        location = TopLoc_Location()
        triangulation = BRep_Tool.Triangulation_s(face, location)
        if triangulation is None:
            continue

        transform = location.Transformation()
        first_vertex = len(vertices)
        for node_index in range(1, triangulation.NbNodes() + 1):
            point = triangulation.Node(node_index).Transformed(transform)
            vertices.append((point.X(), point.Y(), point.Z()))

        reversed_face = face.Orientation() == TopAbs_REVERSED
        for triangle_index in range(1, triangulation.NbTriangles() + 1):
            a, b, c = triangulation.Triangle(triangle_index).Get()
            if reversed_face:
                b, c = c, b
            triangles.append(
                (first_vertex + a - 1, first_vertex + b - 1,
                 first_vertex + c - 1, color)
            )


def convert(input_path: Path, output_path: Path, deflection: float, angle: float):
    reader = STEPControl_Reader()
    status = reader.ReadFile(str(input_path))
    if status != IFSelect_RetDone:
        raise RuntimeError("OpenCASCADE 无法读取该 STEP 文件")
    if reader.TransferRoots() <= 0:
        raise RuntimeError("STEP 文件中没有可转换的实体")

    shape = reader.OneShape()
    if shape.IsNull():
        raise RuntimeError("STEP 文件转换后为空模型")

    mesher = BRepMesh_IncrementalMesh(shape, deflection, False, angle, True)
    mesher.Perform()
    if not mesher.IsDone():
        raise RuntimeError("STEP 曲面三角化失败")

    vertices = []
    triangles = []
    solids = list(iter_shapes(shape, TopAbs_SOLID))
    if solids:
        for solid_index, solid in enumerate(solids):
            append_faces(solid, COLORS[solid_index % len(COLORS)],
                         vertices, triangles)
    else:
        append_faces(shape, COLORS[0], vertices, triangles)

    if not vertices or not triangles:
        raise RuntimeError("STEP 文件没有生成可显示的三角网格")

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("wb") as stream:
        stream.write(struct.pack("<8sIII", MAGIC, VERSION,
                                 len(vertices), len(triangles)))
        for vertex in vertices:
            stream.write(struct.pack("<fff", *vertex))
        for a, b, c, color in triangles:
            stream.write(struct.pack("<III4B", a, b, c, *color))

    print(
        f"OK vertices={len(vertices)} triangles={len(triangles)} "
        f"solids={len(solids)} output={output_path}",
        flush=True,
    )


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--deflection", type=float, default=1.0)
    parser.add_argument("--angle", type=float, default=0.35)
    args = parser.parse_args()

    if not args.input.is_file():
        raise FileNotFoundError(f"找不到 STEP 文件: {args.input}")
    convert(args.input.resolve(), args.output.resolve(),
            max(args.deflection, 0.01), max(args.angle, 0.05))


if __name__ == "__main__":
    try:
        main()
    except Exception as exc:
        print(f"ERROR: {exc}", file=sys.stderr, flush=True)
        sys.exit(1)
