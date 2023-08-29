export IDF_TOOLS_PATH=`pwd`/esp-idf-tools

get_idf() {
	export IDF_PATH=$PWD/esp-idf
	. ${IDF_PATH}/export.sh
	idf="$ESP_PYTHON $IDF_PATH/tools/idf.py"
	esptool="$ESP_PYTHON $IDF_PATH/components/esptool_py/esptool/esptool.py"
	espsecure="$ESP_PYTHON $IDF_PATH/components/esptool_py/esptool/espsecure.py"
	espefuse="$ESP_PYTHON $IDF_PATH/components/esptool_py/esptool/espefuse.py"
}

print_common_usage() {
	echo "usage: $0 -p [port]"
	echo "note: you can define default ports in the defport-monitor and defport-flash files"
	exit 1
}

parse_args() {
	while getopts "p:kf" o; do
	    case "${o}" in
			p)
				port=${OPTARG}
				;;
			f)
				force=1
				;;
			*)
				print_common_usage
				;;
	    esac
	done
	shift $((OPTIND-1))

	if [ -z "$port_monitor" ] && [ -e defport-monitor ]; then
		port_monitor=`cat defport-monitor`
	fi
	if [ -z "$port_flash" ] && [ -e defport-flash ]; then
		port_flash=`cat defport-flash`
	fi
	if [ -z "$port_monitor" ] || [ -z "$port_flash" ]; then
		print_common_usage
	fi
}

reset_build_ts() {
	rm -f build/esp-idf/app_update/CMakeFiles/__idf_app_update.dir/esp_app_desc.c.obj
}

do_clean() {
	$idf clean
	$idf fullclean
	rm -rf $scriptdir/build
}
