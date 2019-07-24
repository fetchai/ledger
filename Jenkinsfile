DOCKER_IMAGE_NAME = 'gcr.io/organic-storm-201412/fetch-ledger-develop:v0.3.0'
HIGH_LOAD_NODE_LABEL = 'ledger'
MACOS_NODE_LABEL = 'mac-mini'

enum Platform
{
  DEFAULT_CLANG('Clang',      'clang',     'clang++'    ),
  CLANG6       ('Clang 6',    'clang-6.0', 'clang++-6.0'),
  CLANG7       ('Clang 7',    'clang-7',   'clang++-7'  ),
  GCC7         ('GCC 7',      'gcc-7',     'g++-7'      ),
  GCC8         ('GCC 8',      'gcc-8',     'g++-8'      )

  public Platform(label, cc, cxx)
  {
    this.label = label
    this.env_cc = cc
    this.env_cxx = cxx
  }

  public final String label
  public final String env_cc
  public final String env_cxx
}

LINUX_PLATFORMS_CORE = [
  Platform.CLANG6,
  Platform.GCC7]

LINUX_PLATFORMS_AUX = [
  Platform.CLANG7,
  Platform.GCC8]

enum Configuration
{
  DEBUG  ('Debug'),
  RELEASE('Release')

  public Configuration(label)
  {
    this.label = label
  }

  public final String label
}

def is_master_or_pull_request_head_branch()
{
  return BRANCH_NAME == 'master' || BRANCH_NAME ==~ /^PR-\d+-head$/
      || BRANCH_NAME ==~ /^PR-\d+$/
}

def static_analysis()
{
  return {
    stage('Static Analysis') {
      node {
        stage('SCM Static Analysis') {
          checkout scm
        }

        stage('Run Static Analysis') {
          docker.image(DOCKER_IMAGE_NAME).inside {
            sh '''\
              mkdir -p build-analysis
              cd build-analysis
              cmake ..
            '''
            sh './scripts/run_static_analysis.py build-analysis'
          }
        }
      }
    }
  }
}

def stage_name_suffix(Platform platform, Configuration config)
{
  return "${platform.label} ${config.label}"
}

def build_stage(Platform platform, Configuration config)
{
  return {
    stage("Build ${stage_name_suffix(platform, config)}") {
      sh "./scripts/ci-tool.py -B ${config.label}"
    }
  }
}

def fast_tests_stage(Platform platform, Configuration config)
{
  return {
    stage("Unit Tests ${stage_name_suffix(platform, config)}") {
      sh "./scripts/ci-tool.py -T ${config.label}"
    }

    stage("Etch Lang Tests ${stage_name_suffix(platform, config)}") {
      sh "./scripts/ci-tool.py -L ${config.label}"
    }
  }
}

def slow_tests_stage(Platform platform, Configuration config)
{
  return {
    stage("Slow Tests ${stage_name_suffix(platform, config)}") {
      sh "./scripts/ci-tool.py -S ${config.label}"
    }

    stage("Integration Tests ${stage_name_suffix(platform, config)}") {
      sh "./scripts/ci-tool.py -I ${config.label}"
    }

    stage("End-to-End Tests ${stage_name_suffix(platform, config)}") {
      sh './scripts/ci/install-test-dependencies.sh'
      sh "./scripts/ci-tool.py -E ${config.label}"
    }
  }
}

def _create_build(
  Platform platform,
  Configuration config,
  node_label,
  stages_fn,
  run_build_fn)
{
  def environment = {
    return [
      "CC=${platform.env_cc}",
      "CXX=${platform.env_cxx}"
    ]
  }

  return {
    stage(stage_name_suffix(platform, config)) {
      node(node_label) {
        timeout(120) {
          stage("SCM ${stage_name_suffix(platform, config)}") {
            checkout scm
          }

          withEnv(environment()) {
            run_build_fn(stages_fn(platform, config))
          }
        }
      }
    }
  }
}

fast_run = { platform_, config_ ->
  return {
    build_stage(platform_, config_)()
    fast_tests_stage(platform_, config_)()
  }
}

full_run = { platform_, config_ ->
  return {
    build_stage(platform_, config_)()
    fast_tests_stage(platform_, config_)()
    slow_tests_stage(platform_, config_)()
  }
}


def create_docker_build(Platform platform, Configuration config, stages)
{
  def build = { build_stages ->
    docker.image(DOCKER_IMAGE_NAME).inside {
      build_stages()
    }
  }

  return _create_build(
    platform,
    config,
    HIGH_LOAD_NODE_LABEL,
    stages,
    build)
}

def create_macos_build(Platform platform, Configuration config)
{
  def build = { build_stages -> build_stages() }

  return _create_build(
    platform,
    config,
    MACOS_NODE_LABEL,
    fast_run,
    build)
}

def run_builds_in_parallel()
{
  def stages = [:]

  for (config in Configuration.values())
  {
    for (platform in LINUX_PLATFORMS_CORE)
    {
      stages["${platform.label} ${config.label}"] = create_docker_build(
        platform,
        config,
        full_run)
    }

    // Only run macOS builds on master and head branches
    if (is_master_or_pull_request_head_branch())
    {
      stages["macOS ${Platform.DEFAULT_CLANG.label} ${config.label}"] = create_macos_build(
        Platform.DEFAULT_CLANG,
        config)
    }
  }

  for (config in (is_master_or_pull_request_head_branch() ? Configuration.values() : [Configuration.RELEASE]))
  {
    for (platform in LINUX_PLATFORMS_AUX)
    {
      stages["${platform.label} ${config.label}"] = create_docker_build(
        platform,
        config,
        is_master_or_pull_request_head_branch() ? full_run : fast_run)
    }
  }

  if (is_master_or_pull_request_head_branch())
  {
    stages['Static Analysis'] = static_analysis()
  }

  stage('Build and Test') {
    // Execute stages
    parallel stages
  }
}

def run_basic_checks()
{
  stage('Basic Checks') {
    node {
      stage('SCM Basic Checks') {
        checkout scm
      }

      docker.image(DOCKER_IMAGE_NAME).inside {
        stage('Style Check') {
          sh './scripts/apply_style.py -d'
        }
      }
    }
  }
}

def main()
{
  timeout(180) {
    run_basic_checks()
    run_builds_in_parallel()
  }
}

// Entry point
main()
