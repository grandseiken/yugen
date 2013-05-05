// Magic include file doesn't need preprocessor guards. Types and functions
// defined here with the ylib macros are automatically exposed to Lua.

ylib_typedef(Filesystem);
ylib_typedef(Script);

ylib_api(hello) ylib_checked_arg(y::int32, t, t < 100, "pass less than 100")
{
  std::cout << "hello you passed " << t << std::endl;
  ylib_void();
}

ylib_api(twice) ylib_arg(y::int32, t)
{
  ylib_return(2 * t, 3 * t);
}

ylib_api(test)
{
  y::vector<y::int32> t;
  t.push_back(17);
  t.push_back(2);
  ylib_return(t);
}

ylib_api(print_vector) ylib_arg(y::vector<y::int32>, t)
{
  for (y::size i = 0; i < t.size(); ++i) {
    std::cout << "an element " << t[i] << std::endl;
  }
  ylib_void();
}

ylib_api(pass_script) ylib_arg(Script*, t)
{
  (void)t;
  ylib_void();
}

ylib_api(get_file)
{
  ylib_return((Filesystem*)y::null);
}

ylib_api(get_script)
{
  ylib_return((Script*)y::null);
}
