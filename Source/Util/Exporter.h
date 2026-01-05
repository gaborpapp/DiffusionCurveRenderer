#pragma once

#include "Curve/Curve.h"

namespace DiffusionCurveRenderer
{
    class Exporter
    {
      public:
        Exporter() = delete;

        static bool ExportAsJson(QVector<CurvePtr> curves, const QString& filename);
        static bool ExportAsXml(QVector<CurvePtr> curves, const QString& filename, int imageWidth = 512, int imageHeight = 512);
    };
}