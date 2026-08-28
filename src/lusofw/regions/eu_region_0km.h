#pragma once

#include <lusofw/AutoRegions.h>

// Macro-polygon to quickly determine if a coordinate falls roughly within the European continent.
// This is intentionally coarse (10 points) to save memory and CPU cycles while providing
// enough accuracy to exclude North Africa and cover the Canary Islands and Azores.
const GeoPoint EUROPE_BASE_POLYGON[] = {
    { 75.0f, -35.0f }, // NW (Iceland/Greenland sea)
    { 75.0f, 65.0f },  // NE (Ural mountains north)
    { 40.0f, 65.0f },  // SE (Caspian sea area)
    { 35.0f, 45.0f },  // East Med (Syria/Turkey border area)
    { 33.0f, 35.0f },  // Below Cyprus
    { 33.0f, 20.0f },  // Below Crete/Greece
    { 35.0f, 10.0f },  // Between Sicily and Tunisia
    { 35.0f, -6.0f },  // Strait of Gibraltar
    { 27.0f, -20.0f }, // Below Canary Islands
    { 35.0f, -35.0f }  // SW (covers Azores and Madeira)
};

const RegionPolygon REGION_EUROPE = { "#europe", EUROPE_BASE_POLYGON, 10 };
