for %%r in (0_1 0_4 0 1 2 3 4 5) do (
rem    ffmpeg -hide_banner -framerate 60 -i raw\div_bg_%%r_%%04d.png ^
rem        -c:v prores_ks -profile:v 4 -vendor apl0 -bits_per_mb 8000 -pix_fmt yuva444p10le -y %%r_prores.mov
rem    ffmpeg -hide_banner -framerate 60 -i raw\div_bg_%%r_%%04d.png ^
rem        -c:v libvpx-vp9 -pix_fmt yuva420p -y %%r.webm
    ffmpeg -hide_banner -framerate 60 -i raw\div_bg_%%r-%%04d.png ^
        -c:v libvpx-vp9 -pix_fmt yuv420p -y div_bg_%%r.webm
)