if [ -r /etc/maginat-lang ]; then
  ML=$(cat /etc/maginat-lang)
else
  ML=en
  case " $(cat /proc/cmdline 2>/dev/null) " in *" maginat.lang=it "*) ML=it;; *" maginat.lang=es "*) ML=es;; esac
fi
case "$ML" in
  it) export LANG=it_IT.UTF-8;;
  es) export LANG=es_ES.UTF-8;;
  *) export LANG=en_US.UTF-8;;
esac
unset ML
