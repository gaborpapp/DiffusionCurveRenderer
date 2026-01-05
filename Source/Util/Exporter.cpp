#include "Exporter.h"

#include "Curve/Spline.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDomDocument>
#include <QTextStream>

bool DiffusionCurveRenderer::Exporter::ExportAsJson(QVector<CurvePtr> curves, const QString& filename)
{
    QJsonArray splines;
    QJsonArray beziers;

    for (const auto& curve : curves)
    {
        if (SplinePtr spline = std::dynamic_pointer_cast<Spline>(curve))
        {
            splines.append(spline->ToJsonObject());
        }
        else if (BezierPtr bezier = std::dynamic_pointer_cast<Bezier>(curve))
        {
            beziers.append(bezier->ToJsonObject());
        }
    }

    QJsonObject root;

    root.insert("spline_curves", splines);
    root.insert("bezier_curves", beziers);

    QByteArray bytes = QJsonDocument(root).toJson(QJsonDocument::Indented);

    QFile file;
    file.setFileName(filename);

    if (file.open(QIODevice::WriteOnly) == false)
    {
        qFatal() << "Exporter::Export: No write access to file '{}'" << filename.toStdString();
        return false;
    }

    file.write(bytes);
    file.close();

    return true;
}

bool DiffusionCurveRenderer::Exporter::ExportAsXml(QVector<CurvePtr> curves, const QString& filename, int imageWidth, int imageHeight)
{
    QDomDocument doc;

    // Add DOCTYPE
    QDomNode xmlDeclaration = doc.createProcessingInstruction("xml", "version=\"1.0\" encoding=\"UTF-8\"");
    doc.appendChild(xmlDeclaration);

    QDomDocumentType docType = doc.implementation().createDocumentType("CurveSetXML", "", "");
    doc.appendChild(docType);

    // Create root element
    QDomElement root = doc.createElement("curve_set");
    root.setAttribute("image_width", imageWidth);
    root.setAttribute("image_height", imageHeight);

    // Count only Bezier curves (XML format doesn't support Splines)
    int bezierCount = 0;
    for (const auto& curve : curves)
    {
        if (std::dynamic_pointer_cast<Bezier>(curve))
            bezierCount++;
    }
    root.setAttribute("nb_curves", bezierCount);
    doc.appendChild(root);

    // Export each curve
    for (const auto& curve : curves)
    {
        BezierPtr bezier = std::dynamic_pointer_cast<Bezier>(curve);
        if (!bezier)
        {
            qWarning() << "Exporter::ExportAsXml: Skipping non-Bezier curve (XML format only supports Bezier)";
            continue;
        }

        QDomElement curveElement = doc.createElement("curve");

        int nbControlPoints = bezier->GetNumberOfControlPoints();
        int nbLeftColors = bezier->GetNumberOfLeftColors();
        int nbRightColors = bezier->GetNumberOfRightColors();
        int nbBlurPoints = bezier->GetNumberOfBlurPoints();

        curveElement.setAttribute("nb_control_points", nbControlPoints);
        curveElement.setAttribute("nb_left_colors", nbLeftColors);
        curveElement.setAttribute("nb_right_colors", nbRightColors);
        curveElement.setAttribute("nb_blur_points", nbBlurPoints);
        curveElement.setAttribute("lifetime", 40); // Default value from sample files

        // Calculate maxGlobalID: 10 * number of cubic segments
        // For Bezier: segments = (nb_control_points - 1) / 3
        int numSegments = qMax(1, (nbControlPoints - 1) / 3);
        int maxGlobalID = 10 * numSegments;

        // Control points (swap x/y to match import format)
        QDomElement controlPointsSet = doc.createElement("control_points_set");
        for (const auto& cp : bezier->GetControlPoints())
        {
            QDomElement cpElement = doc.createElement("control_point");
            cpElement.setAttribute("x", QString::number(cp->position.y(), 'g', 15));
            cpElement.setAttribute("y", QString::number(cp->position.x(), 'g', 15));
            controlPointsSet.appendChild(cpElement);
        }
        curveElement.appendChild(controlPointsSet);

        // Left colors
        QDomElement leftColorsSet = doc.createElement("left_colors_set");
        for (const auto& colorPoint : bezier->GetColorPoints())
        {
            if (colorPoint->type != ColorPointType::Left)
                continue;

            QDomElement colorElement = doc.createElement("left_color");
            // Convert from 0.0-1.0 to 0-255, swap R and B to match import format
            colorElement.setAttribute("R", static_cast<int>(colorPoint->color.z() * 255.0f));
            colorElement.setAttribute("G", static_cast<int>(colorPoint->color.y() * 255.0f));
            colorElement.setAttribute("B", static_cast<int>(colorPoint->color.x() * 255.0f));
            // Convert position to globalID
            colorElement.setAttribute("globalID", static_cast<int>(colorPoint->position * maxGlobalID));
            leftColorsSet.appendChild(colorElement);
        }
        curveElement.appendChild(leftColorsSet);

        // Right colors
        QDomElement rightColorsSet = doc.createElement("right_colors_set");
        for (const auto& colorPoint : bezier->GetColorPoints())
        {
            if (colorPoint->type != ColorPointType::Right)
                continue;

            QDomElement colorElement = doc.createElement("right_color");
            // Convert from 0.0-1.0 to 0-255, swap R and B to match import format
            colorElement.setAttribute("R", static_cast<int>(colorPoint->color.z() * 255.0f));
            colorElement.setAttribute("G", static_cast<int>(colorPoint->color.y() * 255.0f));
            colorElement.setAttribute("B", static_cast<int>(colorPoint->color.x() * 255.0f));
            // Convert position to globalID
            colorElement.setAttribute("globalID", static_cast<int>(colorPoint->position * maxGlobalID));
            rightColorsSet.appendChild(colorElement);
        }
        curveElement.appendChild(rightColorsSet);

        // Blur points
        QDomElement blurPointsSet = doc.createElement("blur_points_set");
        for (const auto& blurPoint : bezier->GetBlurPoints())
        {
            QDomElement blurElement = doc.createElement("best_scale");
            blurElement.setAttribute("value", static_cast<int>(blurPoint->strength));
            blurElement.setAttribute("globalID", static_cast<int>(blurPoint->position * maxGlobalID));
            blurPointsSet.appendChild(blurElement);
        }
        curveElement.appendChild(blurPointsSet);

        root.appendChild(curveElement);
    }

    // Write to file
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
        qFatal() << "Exporter::ExportAsXml: No write access to file" << filename;
        return false;
    }

    QTextStream stream(&file);
    stream << doc.toString(2); // Indent with 2 spaces
    file.close();

    qDebug() << "Exporter::ExportAsXml: Exported" << bezierCount << "curves to" << filename;
    return true;
}