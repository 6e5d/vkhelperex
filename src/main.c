#include <unistd.h>
#include <vulkan/vulkan.h>
#include <wayland-client.h>

#include "../../chrono/build/chrono.h"
#include "../../vkstatic/build/vkstatic.h"
#include "../../vkhelper2/build/vkhelper2.h"
#include "../../vkbasic/build/vkbasic.h"
#include "../../vkwayland/build/vkwayland.h"
#include "../../wlezwrap/build/wlezwrap.h"

static void event(void *data, uint8_t ty, Wlezwrap(Event)*) {
	bool *quit = data;
	if (ty == 0) { *quit = true; }
}

int main(void) {
	Vkstatic() vs;
	Vkbasic() vb;
	Wlezwrap() wew = {0};

	bool quit = false;
	wlezwrap(confgen)(&wew);
	wew.event = event;
	wew.data = (void*)&quit;
	wlezwrap(init)(&wew);
	vkwayland(new)(&vs, wew.wl.display, wew.wl.surface);
	vkbasic(init)(&vb, vs.device);

	VkRenderPass rp;
	Vkhelper2(RenderpassConfig) rpconf;
	vkhelper2(renderpass_config)(&rpconf,
		vs.surface_format.format, vs.depth_format);
	vkhelper2(renderpass_build)(&rp, &rpconf, vs.device);
	vkbasic(swapchain_update)(&vb, &vs, rp, 640, 480);

	VkPipelineLayout ppll;
	VkPipeline ppl;
	Vkhelper2(PipelineConfig) vpc = {0};
	vkhelper2(pipeline_config)(&vpc, 0, 0, 0);
	vkhelper2(pipeline_simple_shader)(&vpc, vs.device,
		__FILE__, "../../vwdraw_shaders/build/grid");
	vkhelper2(pipeline_build)(&ppll, &ppl, &vpc, rp, vs.device, 0);
	vkhelper2(pipeline_config_deinit)(&vpc, vs.device);
	uint32_t index = 0;

	while(!quit) {
		VkFramebuffer fb = vkbasic(next_index)(&vb, vs.device, &index);
		VkCommandBuffer cbuf = vkstatic(begin)(&vs);
		vkhelper2(dynstate_vs)(cbuf, 640, 480);
		vkhelper2(renderpass_begin)(cbuf, rp, fb, 640, 480);
		vkCmdBindPipeline(cbuf, VK_PIPELINE_BIND_POINT_GRAPHICS, ppl);
		vkCmdDraw(cbuf, 6, 1, 0, 0);
		vkCmdEndRenderPass(cbuf);
		assert(0 == vkEndCommandBuffer(cbuf));
		vkbasic(submit)(&vb, vs.queue, cbuf);
		vkbasic(present)(&vb, vs.queue, &index);

		wl_display_roundtrip(wew.wl.display);
		wl_display_dispatch_pending(wew.wl.display);
		com_6e5d_chrono_sleep(10000000);
	}

	vkDeviceWaitIdle(vs.device);
	vkDestroyPipeline(vs.device, ppl, NULL);
	vkDestroyPipelineLayout(vs.device, ppll, NULL);
	vkDestroyRenderPass(vs.device, rp, NULL);
	vkbasic(deinit)(&vb, vs.device);
	vkstatic(deinit)(&vs);
	wlezwrap(deinit)(&wew);
}
