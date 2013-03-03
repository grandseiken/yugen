#ifndef CELL_MAP_H
#define CELL_MAP_H

#include "cell.h"

class CellBlueprint;

struct CellCoord {

  CellCoord(y::int32 x, y::int32 y);

  bool operator==(const CellCoord& cell_coord) const;
  bool operator!=(const CellCoord& cell_coord) const;

  y::int32 x;
  y::int32 y;

};

namespace std {

  template<>
  struct hash<CellCoord> {
    y::size operator()(const CellCoord& cell_coord) const;
  };

}

class CellMap {
public:

  bool is_coord_used(const CellCoord& cell_coord) const;

  void clear_coord(const CellCoord& cell_coord);

  void set_coord(const CellCoord& cell_coord, CellBlueprint& blueprint);

  const CellBlueprint* get_coord(const CellCoord& cell_coord) const;
  /***/ CellBlueprint* get_coord(const CellCoord& cell_coord);

private:

  y::map<CellCoord, CellBlueprint*> _map;

};

#endif
