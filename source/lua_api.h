// Magic include file doesn't need preprocessor guards.

ylib_api(hello) ylib_arg(y::int32, t)
{
  std::cout << "hello you passed " << t << std::endl;
  ylib_void();
}

ylib_api(twice) ylib_arg(y::int32, t)
{
  ylib_return(2 * t, 3 * t);
}
