
export function get(req, res, next) {
  const units = ['mgd','fps'];
  res.json(units);
}
