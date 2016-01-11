#### import the simple module from the paraview
from paraview.simple import *
#### disable automatic camera reset on 'Show'
paraview.simple._DisableFirstRenderCameraReset()

# find source
clip = FindSource('Clip')
SetActiveSource(clip)

# Properties modified on clip
clip.Origin = [0.0, 0.0, 0.0]
clip.Normal = [0.0, 0.0, -1.0]

# find source
contour = FindSource('Contour')
SetActiveSource(contour)

# Properties modified on contour
contour.ContourField = 'Density'
contour.Isosurfaces = [0.735]
contour.ColorField = 'Velocity_u'
contour.ColorPalette = 'Cool to Warm'
contour.ColorRange = [-0.25, 0.25]


# find source
slice = FindSource('Slice')
SetActiveSource(slice)


# Properties modified on slice
slice.NumberOfPlanes = 1
slice.Spacing = 0.0
slice.Origin = [0.0, 0.0, 0.002]
slice.Normal = [0.0, 0.0, 1.0]
slice.ColorField = 'Density'
slice.ColorPalette = 'Cool to Warm'
slice.ColorRange = [0.735, 0.745]

#### uncomment the following to render all views
# RenderAllViews()
# alternatively, if you want to write images, you can use SaveScreenshot(...).