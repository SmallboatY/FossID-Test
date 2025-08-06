static size_t fwk_module_count_elements(const struct fwk_element *elements)
{
    size_t count = 0;

    for (; elements[count].name != NULL; count++) {
        continue;
    }

    return count;
}

#ifdef BUILD_HAS_NOTIFICATION
static void fwk_module_init_subscriptions(struct fwk_dlist **list, size_t count)
{
    *list = fwk_mm_calloc(count, sizeof((*list)[0]));
    if (*list == NULL) {
        fwk_trap();
    }

    for (size_t i = 0; i < count; i++) {
        fwk_list_init(&((*list)[i]));
    }
}
#endif

static void fwk_module_init_element_ctx(
    struct fwk_element_ctx *ctx,
    const struct fwk_element *element,
    size_t notification_count)
{
    *ctx = (struct fwk_element_ctx){
        .state = FWK_MODULE_STATE_UNINITIALIZED,
        .desc = element,
        .sub_element_count = element->sub_element_count,
    };

    fwk_list_init(&ctx->delayed_response_list);

#ifdef BUILD_HAS_NOTIFICATION
    if (notification_count > 0) {
        fwk_module_init_subscriptions(
            &ctx->subscription_dlist_table, notification_count);
    }
#endif
}

static void fwk_module_init_element_ctxs(
    struct fwk_module_context *ctx,
    const struct fwk_element *elements,
    size_t notification_count)
{
    ctx->element_count = fwk_module_count_elements(elements);

    ctx->element_ctx_table =
        fwk_mm_calloc(ctx->element_count, sizeof(ctx->element_ctx_table[0]));
    if (ctx->element_ctx_table == NULL) {
        fwk_trap();
    }

    for (size_t i = 0; i < ctx->element_count; i++) {
        fwk_module_init_element_ctx(
            &ctx->element_ctx_table[i], &elements[i], notification_count);
    }
}

void fwk_module_init(void)
{
    /*
     * The loop index i gets the values corresponding to the
     * enum fwk_module_idx
     */
    for (uint32_t i = 0U; i < (uint32_t)FWK_MODULE_IDX_COUNT; i++) {
        struct fwk_module_context *ctx = &fwk_module_ctx.module_ctx_table[i];

        fwk_id_t id = FWK_ID_MODULE(i);

        const struct fwk_module *desc = module_table[i];
        const struct fwk_module_config *config = module_config_table[i];

        *ctx = (struct fwk_module_context){
            .id = id,

            .desc = desc,
            .config = config,
        };

        fwk_assert(ctx->desc != NULL);
        fwk_assert(ctx->config != NULL);

        fwk_list_init(&ctx->delayed_response_list);

        if (config->elements.type == FWK_MODULE_ELEMENTS_TYPE_STATIC) {
            size_t notification_count = 0;

#ifdef BUILD_HAS_NOTIFICATION
            notification_count = desc->notification_count;
#endif

            fwk_module_init_element_ctxs(
                ctx, config->elements.table, notification_count);
        }

#ifdef BUILD_HAS_NOTIFICATION
        if (desc->notification_count > 0) {
            fwk_module_init_subscriptions(
                &ctx->subscription_dlist_table, desc->notification_count);
        }
#endif
    }
}


static int fwk_module_bind_module(
    struct fwk_module_context *fwk_mod_ctx,
    unsigned int round)
{
    int status;
    const struct fwk_module *module;

    module = fwk_mod_ctx->desc;
    if (module->bind == NULL) {
        fwk_mod_ctx->state = FWK_MODULE_STATE_BOUND;
        return FWK_SUCCESS;
    }

    fwk_module_ctx.bind_id = fwk_mod_ctx->id;
    status = module->bind(fwk_mod_ctx->id, round);
    if (!fwk_expect(status == FWK_SUCCESS)) {
        FWK_LOG_CRIT(fwk_module_err_msg_func, status, __func__);
        return status;
    }

    if (round == FWK_MODULE_BIND_ROUND_MAX) {
        fwk_mod_ctx->state = FWK_MODULE_STATE_BOUND;
    }

    return fwk_module_bind_elements(fwk_mod_ctx, round);
}

static int fwk_module_bind_modules(unsigned int round)
{
    int status;
    unsigned int module_idx;
    struct fwk_module_context *fwk_mod_ctx;

    for (module_idx = 0; module_idx < FWK_MODULE_IDX_COUNT; module_idx++) {
        fwk_mod_ctx = &fwk_module_ctx.module_ctx_table[module_idx];
        status = fwk_module_bind_module(fwk_mod_ctx, round);
        if (status != FWK_SUCCESS) {
            return status;
        }
    }

    return FWK_SUCCESS;
}
