module C = Configurator.V1

let ( / ) = Filename.concat

let package = "solo5-bindings-xen"

let () =
  C.main ~name:"foo" (fun c ->
      let opam_prefix =
        String.trim
          (C.Process.run_capture_exn c "opam" [ "config"; "var"; "prefix" ])
      in
      Unix.putenv "PKG_CONFIG_PATH" (opam_prefix / "lib" / "pkgconfig");
      let conf =
        match C.Pkg_config.get c with
        | None -> failwith "cannot find pkg-config"
        | Some pc -> (
            match C.Pkg_config.query pc ~package with
            | None -> failwith "cannot find xen's pkg-config info"
            | Some deps -> deps )
      in
      C.Flags.write_sexp "xen_bindings_cflags.sxp" conf.cflags)
