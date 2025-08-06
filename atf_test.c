void bl31_main(void)
{
	unsigned int core_pos = plat_my_core_pos();

	/* Init registers that never change for the lifetime of TF-A */
	cm_manage_extensions_el3(core_pos);

	/* Init per-world context registers */
	cm_manage_extensions_per_world();

	NOTICE("BL31: %s\n", build_version_string);
	NOTICE("BL31: %s\n", build_message);

#if ENABLE_RUNTIME_INSTRUMENTATION
	PMF_CAPTURE_TIMESTAMP(bl_svc, BL31_ENTRY, PMF_CACHE_MAINT);
#endif

#ifdef SUPPORT_UNKNOWN_MPID
	if (unsupported_mpid_flag == 0) {
		NOTICE("Unsupported MPID detected!\n");
	}
#endif

	/* Perform platform setup in BL31 */
	bl31_platform_setup();

#if USE_DSU_DRIVER
	dsu_driver_init(&plat_dsu_data);
#endif

#if USE_GIC_DRIVER
	/*
	 * Initialize the GIC driver as well as per-cpu and global interfaces.
	 * Platform has had an opportunity to initialise specifics.
	 */
	gic_init(core_pos);
	gic_pcpu_init(core_pos);
	gic_cpuif_enable(core_pos);
#endif /* USE_GIC_DRIVER */

	/* Initialise helper libraries */
	bl31_lib_init();

#if EL3_EXCEPTION_HANDLING
	INFO("BL31: Initialising Exception Handling Framework\n");
	ehf_init();
#endif

	/* Initialize the runtime services e.g. psci. */
	INFO("BL31: Initializing runtime services\n");
	runtime_svc_init();

	/*
	 * All the cold boot actions on the primary cpu are done. We now need to
	 * decide which is the next image and how to execute it.
	 * If the SPD runtime service is present, it would want to pass control
	 * to BL32 first in S-EL1. In that case, SPD would have registered a
	 * function to initialize bl32 where it takes responsibility of entering
	 * S-EL1 and returning control back to bl31_main. Similarly, if RME is
	 * enabled and a function is registered to initialize RMM, control is
	 * transferred to RMM in R-EL2. After RMM initialization, control is
	 * returned back to bl31_main. Once this is done we can prepare entry
	 * into BL33 as normal.
	 */

	/*
	 * If SPD had registered an init hook, invoke it.
	 */
	if (bl32_init != NULL) {
		INFO("BL31: Initializing BL32\n");

		console_flush();
		int32_t rc = (*bl32_init)();

		if (rc == 0) {
			WARN("BL31: BL32 initialization failed\n");
		}
	}

	/*
	 * If RME is enabled and init hook is registered, initialize RMM
	 * in R-EL2.
	 */
#if ENABLE_RME
	if (rmm_init != NULL) {
		INFO("BL31: Initializing RMM\n");

		console_flush();
		int32_t rc = (*rmm_init)();

		if (rc == 0) {
			WARN("BL31: RMM initialization failed\n");
		}
	}
#endif

	/*
	 * We are ready to enter the next EL. Prepare entry into the image
	 * corresponding to the desired security state after the next ERET.
	 */
	bl31_prepare_next_image_entry();

	/*
	 * Perform any platform specific runtime setup prior to cold boot exit
	 * from BL31
	 */
	bl31_plat_runtime_setup();

#if ENABLE_RUNTIME_INSTRUMENTATION
	console_flush();
	PMF_CAPTURE_TIMESTAMP(bl_svc, BL31_EXIT, PMF_CACHE_MAINT);
#endif

	console_flush();
	console_switch_state(CONSOLE_FLAG_RUNTIME);
}
