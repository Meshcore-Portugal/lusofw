#pragma once

// Macro-polygon to quickly determine if a coordinate falls roughly within the European continent.
// This is intentionally coarse (14 points) to save memory and CPU cycles while providing
// enough accuracy to exclude North Africa and cover the Canary Islands and Azores.
// The southern boundary routes below the whole Canary archipelago (El Hierro ~27.7N
// to Lanzarote ~29.0N across -18.2..-13.4) via an offshore waypoint so it also stays
// west of the Moroccan Atlantic coast (Casablanca/Agadir) and above the Western
// Sahara coast while enclosing the islands.
static const GeoPoint poly_europe[] = {
    { 75.0f, -35.0f }, // NW (Iceland/Greenland sea)
    { 75.0f,  65.0f }, // NE (Ural mountains north)
    { 40.0f,  65.0f }, // SE (Caspian sea area)
    { 35.0f,  45.0f }, // East Med (Syria/Turkey border area)
    { 33.0f,  35.0f }, // Below Cyprus
    { 33.0f,  20.0f }, // Below Crete/Greece
    { 35.0f,  10.0f }, // Between Sicily and Tunisia
    { 35.0f,  -3.5f }, // West Med, level with the north Moroccan coast (Melilla stays inside)
    { 35.85f, -5.4f }, // Strait of Gibraltar: rise to the strait midline (Ceuta stays inside)
    { 35.85f, -6.1f }, // Strait exit: level past Cape Spartel before descending (Tangier outside)
    { 31.0f, -11.5f }, // Atlantic, off the Moroccan coast (Casablanca/Agadir stay outside)
    { 27.5f, -13.5f }, // Between Lanzarote and the Western Sahara coast
    { 27.0f, -20.0f }, // Below the Canary Islands (whole archipelago inside)
    { 35.0f, -35.0f }, // SW (covers Azores and Madeira)
    { 75.0f, -35.0f }  // closing vertex (repeat of first)
};

static const RegionRing rings_europe[] = {
    {poly_europe, 16},
};

static const RegionPolygon REGION_EUROPE = { "#europe", rings_europe, 1 };
