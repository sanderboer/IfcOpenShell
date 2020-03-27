/********************************************************************************
 *                                                                              *
 * Copyright 2015 IfcOpenShell and ROOT B.V.                                    *
 *                                                                              *
 * This file is part of IfcOpenShell.                                           *
 *                                                                              *
 * IfcOpenShell is free software: you can redistribute it and/or modify         *
 * it under the terms of the Lesser GNU General Public License as published by  *
 * the Free Software Foundation, either version 3.0 of the License, or          *
 * (at your option) any later version.                                          *
 *                                                                              *
 * IfcOpenShell is distributed in the hope that it will be useful,              *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the                 *
 * Lesser GNU General Public License for more details.                          *
 *                                                                              *
 * You should have received a copy of the Lesser GNU General Public License     *
 * along with this program. If not, see <http://www.gnu.org/licenses/>.         *
 *                                                                              *
 ********************************************************************************/

#include <string>
#include <fstream>
#include <cstdio>
#include <limits>
#include <algorithm>

#include <gp_Pln.hxx>
#include <gp_Trsf.hxx>
#include <gp_Circ.hxx>
#include <gp_Elips.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Edge.hxx>
#include <TopExp_Explorer.hxx>
#include <BRep_Tool.hxx>
#include <BRepAlgo_Section.hxx>
#include <BRepTools.hxx>
#include <BRepAlgoAPI_Section.hxx>
#include <ShapeAnalysis_FreeBounds.hxx>
#include <TopTools_HSequenceOfShape.hxx>

#include <BRepAdaptor_Curve.hxx>
#include <GCPnts_QuasiUniformDeflection.hxx>

#include <Geom_Curve.hxx>
#include <Geom_Line.hxx>
#include <Geom_Plane.hxx>
#include <Geom_Circle.hxx>
#include <Geom_Ellipse.hxx>
#include <gp_Ax22d.hxx>
#include <Standard_Version.hxx>
#include <GeomAPI.hxx>
#include <TopoDS_Wire.hxx>

#include <BRepBuilderAPI_Transform.hxx>

#include "../ifcparse/IfcGlobalId.h"

#include "SvgSerializer.h"

const double PI2 = M_PI * 2.;

bool SvgSerializer::ready() {
	return true;
}

void SvgSerializer::write(path_object& p, const TopoDS_Wire& wire) {
	/* ShapeFix_Wire fix;
	Handle(ShapeExtend_WireData) data = new ShapeExtend_WireData;
	for (TopExp_Explorer edges(result, TopAbs_EDGE); edges.More(); edges.Next()) {
		data->Add(edges.Current());
	}
	fix.Load(data);
	fix.FixReorder();
	fix.FixConnected();
	const TopoDS_Wire fixed_wire = fix.Wire(); */

	bool first = true;
	util::string_buffer path;
	for (TopExp_Explorer edges(wire, TopAbs_EDGE); edges.More(); edges.Next()) {
		const TopoDS_Edge& edge = TopoDS::Edge(edges.Current());

		double u1, u2;
		Handle(Geom_Curve) curve = BRep_Tool::Curve(edge, u1, u2);
		Handle(Geom2d_Curve) curve2d;
		if (curve.IsNull()) {
			TopLoc_Location loc;
			Handle_Geom_Surface surf;
			
			BRep_Tool::CurveOnSurface(edge, curve2d, surf, loc, u1, u2);
			
			if (curve2d.IsNull()) {
				Logger::Error("Failed to obtain 2d and 3d curve from edge");
				continue;
			}

			Handle(Standard_Type) sty = surf->DynamicType();
			if (sty != STANDARD_TYPE(Geom_Plane)) {
				Logger::Error("Non-planar p-curves are not supported by this serializer");
				continue;
			}

			gp_Pln pln = Handle(Geom_Plane)::DownCast(surf)->Pln();
			curve = GeomAPI::To3d(curve2d, pln);
		}

		Handle(Standard_Type) ty = curve->DynamicType();
        bool conical = (ty == STANDARD_TYPE(Geom_Circle) || ty == STANDARD_TYPE(Geom_Ellipse));

		// TODO: ALMOST_THE_SAME utilities in separate header
		bool closed = fabs((u1 + PI2) - u2) < 1.e-9;

        if (conical && closed) {
            if (first) {
                if (ty == STANDARD_TYPE(Geom_Circle)) {
                    Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(curve);
                    double r = circle->Radius();
                    gp_Circ c = circle->Circ();
                    gp_Pnt center = c.Location();
                    path.add("            <circle style=\"stroke:black; fill:none;\" r=\"");
                    radii.push_back(path.add(r));
                    path.add("\" cx=\"");
                    xcoords.push_back(path.add(center.X()));
                    path.add("\" cy=\"");
                    ycoords.push_back(path.add(center.Y()));

                    growBoundingBox(center.X() - r, center.Y() - r);
                    growBoundingBox(center.X() + r, center.Y() + r);

                    first = false;
                    continue;
                } else if (ty == STANDARD_TYPE(Geom_Ellipse)) {
                    Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(curve);
                    gp_Elips e = ellipse->Elips();
                    gp_Pnt center = e.Location();

                    // Write the ellipse with major radius along X axis:
                    path.add("            <ellipse style=\"stroke:black; fill:none;\" rx=\"");
                    radii.push_back(path.add(e.MajorRadius()));
                    path.add("\" ry=\"");
                    radii.push_back(path.add(e.MinorRadius()));
                    path.add("\" cx=\"");
                    xcoords.push_back(path.add(center.X()));
                    path.add("\" cy=\"");
                    ycoords.push_back(path.add(center.Y()));
                    path.add("\"");

                    // Rotate it with "transform":
                    gp_Ax1 major_axis = e.XAxis();
                    double z_rotation = major_axis.Direction().AngleWithRef(gp_Dir(1., 0., 0.), gp_Dir(0., 0., 1.));
                    path.add(" transform=\"rotate(");
                    path.add(z_rotation);
                    path.add(" ");
                    path.add(center.X());
                    path.add(" ");
                    path.add(center.Y());

                    // Bounding box:
                    // More important to have all geometry in bounding box than to be minimal
                    growBoundingBox(center.X() - e.MajorRadius(), center.Y() - e.MajorRadius());
                    growBoundingBox(center.X() + e.MajorRadius(), center.Y() + e.MajorRadius());

                    first = false;
                    continue;
                }
            } else {
                std::stringstream ss;
                ss << "Skipping full circle/ellipse inside aggregated <path> (id "
                   << p.first << ")";
                Logger::Warning(ss.str());
            }
        }

        const bool reversed = edge.Orientation() == TopAbs_REVERSED;

		gp_Pnt p1, p2;
		curve->D0(u1, p1);
		curve->D0(u2, p2);

		if (reversed) {
			std::swap(p1, p2);
		}
				
		if (first) {
            path.add("            <path style=\"stroke:black; fill:none;\" d=\"");
			path.add("M");
			addXCoordinate(path.add(p1.X()));
			path.add(",");
			addYCoordinate(path.add(p1.Y()));

			growBoundingBox(p1.X(), p1.Y());
		}

		growBoundingBox(p2.X(), p2.Y());


		if (ty == STANDARD_TYPE(Geom_Circle) || ty == STANDARD_TYPE(Geom_Ellipse)) {
			Handle(Geom_Conic) conic = Handle(Geom_Conic)::DownCast(curve);
			const bool mirrored = conic->Position().Axis().Direction().Z() < 0;
					
			double r1, r2;
			bool larger_arc_segment = (fmod(u2 - u1 + PI2, PI2) > M_PI);
			bool positive_direction = (u2 > u1);

			if (mirrored != reversed) {
				// In case the local coordinate system is mirrored
				// the direction is reversed.
				positive_direction = !positive_direction;
			}

			gp_Pnt center;
			if (ty == STANDARD_TYPE(Geom_Circle)) {
				Handle(Geom_Circle) circle = Handle(Geom_Circle)::DownCast(curve);
				r1 = r2 = circle->Radius();
				center = circle->Location();
			} else {
				Handle(Geom_Ellipse) ellipse = Handle(Geom_Ellipse)::DownCast(curve);
				r1 = ellipse->MajorRadius();
				r2 = ellipse->MinorRadius();
				center = ellipse->Location();
			}

			// Make sure the arc segment is entirely inside bounding box:
			growBoundingBox(center.X() - r1, center.Y() - r1);
			growBoundingBox(center.X() + r1, center.Y() + r1);
					
			// Calculate the angle between 2d vecs to have signed result
			const gp_Dir& d = conic->Position().XDirection();
			const gp_Dir2d d2(d.X(), d.Y());
			const double ang = d2.Angle(gp::DX2d());

			// Write radii
			path.add(" A");
			addSizeComponent(path.add(r1));
			path.add(",");
			addSizeComponent(path.add(r2));
					
			// Write X-axis rotation
			{ std::stringstream ss; ss << " " << ang << " ";
			path.add(ss.str()); }
					
			// Write large-arc-flag and sweep-flag
			path.add(std::string(1, '0'+static_cast<int>(larger_arc_segment)));
			path.add(",");
			path.add(std::string(1, '0'+static_cast<int>(positive_direction)));
					
			path.add(" ");

			// Write arc end point
			xcoords.push_back(path.add(p2.X()));
			path.add(",");
			ycoords.push_back(path.add(p2.Y()));
		} else if (ty != STANDARD_TYPE(Geom_Line)) {
			BRepAdaptor_Curve crv(edge);
			GCPnts_QuasiUniformDeflection tessellater(crv, settings().deflection_tolerance());
			// NB: Start at 2: 1-based and skip the first point, assume it coincides with p1.
			for (int i = 2; i <= tessellater.NbPoints(); ++i) {
				gp_Pnt pi = tessellater.Value(i);
				path.add(" L");
				xcoords.push_back(path.add(pi.X()));
				path.add(",");
				ycoords.push_back(path.add(pi.Y()));

				growBoundingBox(pi.X(), pi.Y());
			}
		} else {
			// Either a Geom_Line or something unimplemented,
			// drawn as a straight line segment.
			path.add(" L");
			xcoords.push_back(path.add(p2.X()));
			path.add(",");
			ycoords.push_back(path.add(p2.Y()));
		}

		first = false;
	}

	path.add("\"/>\n");
	p.second.push_back(path);
}

SvgSerializer::path_object& SvgSerializer::start_path(IfcUtil::IfcBaseEntity* storey, const std::string& id) {
	SvgSerializer::path_object& p = paths.insert(std::make_pair(storey, path_object()))->second;
	p.first = id;
	return p;
}

void SvgSerializer::write(const IfcGeom::BRepElement<real_t>* o)
{
	IfcUtil::IfcBaseEntity* storey = storey_;
	boost::optional<double> storey_elevation = boost::none;

	for (const auto& p : o->parents()) {
		if (p->type() == "IfcBuildingStorey") {
			try {
				const IfcGeom::ElementSettings& settings = o->geometry().settings();
				double e = *p->product()->get("Elevation");
				storey_elevation = e * settings.unit_magnitude();
			} catch (...) {
				continue;
			}
			storey = p->product();
			break;
		}
	}

	// With a global section height, building storeys are not a requirement.
	if (!storey && !section_height) {
		Logger::Warning("No global section height and unable to determine building storey for:", o->product());
		return;
	}

	path_object& p = start_path(storey, nameElement(o));

	// SVG has a coordinate system with the origin in the *upper*-left corner
	// therefore we mirror the shape along the XZ-plane.
	TopoDS_Shape compound = o->geometry().as_compound();
	gp_Trsf trsf;
	trsf.SetMirror(gp_Ax2(gp::Origin(), gp::DY()));
	BRepBuilderAPI_Transform make_transform(compound, trsf);
	make_transform.Build();
	// When determinant < 0, copy is implied and the input is not mutated.
	compound = make_transform.Shape();

	TopoDS_Iterator it(compound);

	// Iterate over components of compound to have better chance of matching section edges to closed wires
	for (; it.More(); it.Next()) {
		
		const TopoDS_Shape& subshape = it.Value();

		const double inf = std::numeric_limits<double>::infinity();
		double zmin = inf;
		double zmax = -inf;
		{TopExp_Explorer exp(subshape, TopAbs_VERTEX);
		for (; exp.More(); exp.Next()) {
			const TopoDS_Vertex& vertex = TopoDS::Vertex(exp.Current());
			gp_Pnt pnt = BRep_Tool::Pnt(vertex);
			if (pnt.Z() < zmin) { zmin = pnt.Z(); }
			if (pnt.Z() > zmax) { zmax = pnt.Z(); }
		}}
		
		// Empty geometry, no vertices encountered
		if (zmin == inf) continue;
		
		// Determine slicing plane z coordinate, priority:
		// 1) explicitly set global section height
		// 2) containing building storey elevation + 1m
		// 3) zmin (from geometry bounding box) + 1m
		double cut_z;
		if (section_height) {
			cut_z = section_height.get();
		} else if (storey_elevation && !(zmin > *storey_elevation || zmax < *storey_elevation)) {
			cut_z = storey_elevation.get() + 1.;
		} else {
			cut_z = zmin + 1.;
		}

		// No intersection with bounding box, fail early
		if (zmin > cut_z || zmax < cut_z) continue;
        
		// Create a horizontal cross section 1 meter above the bottom point of the shape		
		TopoDS_Shape result = BRepAlgoAPI_Section(subshape, gp_Pln(gp_Pnt(0, 0, cut_z), gp::DZ()));

		Handle(TopTools_HSequenceOfShape) edges = new TopTools_HSequenceOfShape();
		Handle(TopTools_HSequenceOfShape) wires = new TopTools_HSequenceOfShape();
		{TopExp_Explorer exp(result, TopAbs_EDGE);
		for (; exp.More(); exp.Next()) {
			edges->Append(exp.Current());
		}}
		ShapeAnalysis_FreeBounds::ConnectEdgesToWires(edges, 1e-5, false, wires);

		gp_Pnt prev;

		for (int i = 1; i <= wires->Length(); ++i) {
			const TopoDS_Wire& wire = TopoDS::Wire(wires->Value(i));
			write(p, wire);
		}
	}
}

void SvgSerializer::setBoundingRectangle(double width, double height) {
	this->width = width;
	this->height = height;
	this->rescale = true;
}

void SvgSerializer::finalize() {

	if (rescale) {
		// Scale the resulting image to a bounding rectangle specified by command line arguments
		const double dx = xmax - xmin;
		const double dy = ymax - ymin;

		double sc = 1.;

		if (dx / width > dy / height) {
			sc = width / dx;
		} else {
			sc = height / dy;
		}

		const double cx = xmin * sc;
		const double cy = ymin * sc;

		{std::vector< boost::shared_ptr<util::string_buffer::float_item> >::const_iterator it;
		for (it = xcoords.begin(); it != xcoords.end(); ++it) {
			double& v = (*it)->value();
			v = v * sc - cx;
		}
		for (it = ycoords.begin(); it != ycoords.end(); ++it) {
			double& v = (*it)->value();
			v = v * sc - cy;
		}
		for (it = radii.begin(); it != radii.end(); ++it) {
			(*it)->value() *= sc;
		}}
	}

	std::multimap<IfcUtil::IfcBaseEntity*, path_object>::const_iterator it;

	IfcUtil::IfcBaseEntity* previous = 0;
	bool first = true;
	for (it = paths.begin(); it != paths.end(); ++it) {
		if (it->first != previous || first) {
			if (!first) {
				svg_file << "    </g>\n";
			}
			std::ostringstream oss;
			svg_file << "    <g " << nameElement(it->first) << ">\n";
		}
		svg_file << "        <g " << it->second.first << ">\n";
		std::vector<util::string_buffer>::const_iterator jt;
		for (jt = it->second.second.begin(); jt != it->second.second.end(); ++jt) {
			svg_file << jt->str();
		}
		svg_file << "        </g>\n";
		previous = it->first;
		first = false;
	}
	
	if (!first) {
		svg_file << "    </g>\n";
	}
	svg_file << "</svg>" << std::endl;
}

void SvgSerializer::writeHeader() {
	svg_file << "<svg xmlns=\"http://www.w3.org/2000/svg\" xmlns:xlink=\"http://www.w3.org/1999/xlink\">\n";
}

std::string SvgSerializer::nameElement(const IfcGeom::Element<real_t>* elem)
{
	std::ostringstream oss;
	const std::string type = "product";
    const std::string name = object_id(elem);
	oss << "id=\"" << type << "-" << name<< "\"";
	return oss.str();
}

std::string SvgSerializer::nameElement(const IfcUtil::IfcBaseEntity* elem) {
	if (elem == 0) { return ""; }
	std::ostringstream oss;
	const std::string type = elem->declaration().is("IfcBuildingStorey") ? "storey" : "product";

    const std::string name = 
		(settings().get(SerializerSettings::USE_ELEMENT_GUIDS)
			? static_cast<std::string>(*elem->get("GlobalId"))
			: ((settings().get(SerializerSettings::USE_ELEMENT_NAMES) && !elem->get("Name")->isNull()))
				? static_cast<std::string>(*elem->get("Name"))
				: IfcParse::IfcGlobalId(*elem->get("GlobalId")).formatted());

	oss << "id=\"" << type << "-" << name << "\"";
	return oss.str();
}

void SvgSerializer::setFile(IfcParse::IfcFile* f) {
	file = f;

	auto storeys = f->instances_by_type("IfcBuildingStorey");
	if (!storeys || storeys->size() == 0) {
		
		IfcGeom::Kernel kernel(f);
		
		std::vector<const IfcParse::declaration*> to_derive_from;
		to_derive_from.push_back(f->schema()->declaration_by_name("IfcBuilding"));
		to_derive_from.push_back(f->schema()->declaration_by_name("IfcSite"));
		for (auto it = to_derive_from.begin(); it != to_derive_from.end(); ++it) {
			IfcEntityList::ptr insts = f->instances_by_type(*it);
			if (insts) {
				for (auto jt = insts->begin(); jt != insts->end(); ++jt) {
					IfcUtil::IfcBaseEntity* product = (IfcUtil::IfcBaseEntity*) *jt;
					if (!product->get("ObjectPlacement")->isNull()) {
						gp_Trsf trsf;
						if (kernel.convert_placement(*product->get("ObjectPlacement"), trsf)) {
							setSectionHeight(trsf.TranslationPart().Z() + 1.);
							Logger::Warning("No building storeys encountered, used for reference:", product);
							return;
						}
					}
				}
			}
		}

		Logger::Warning("No building storeys encountered, output might be invalid or missing");
	}
}
